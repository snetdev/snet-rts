/* 
 * Stream, implemented as Single-Writer Single-Reader circular buffer.
 * 
 * Locking the whole stream is not needed for the Read and Write methods,
 * but Read synchronises with Open for writing (producer lock)
 * and Write with Open for reading (consumer lock).
 * As opening/closing happens seldom in most cases, there should be no
 * competition for the lock most of the time.
 *
 * It uses ideas from the FastForward queue implementation:
 * Synchronisation takes place with the content of the buffer, i.e., NULL indicates that
 * the location is empty. After reading a value, the consumer writes NULL back to the
 * position it read the data from.
 *
 * A task attempting to read from an empty stream will context switch after setting
 * its state to WAITING on write and pointing the event pointer to the location
 * in the buffer it waits the producer to write something into.
 *
 * Same holds for a task attempting to write to a full stream, except it changes state
 * to WAITING on read, it has to be woken up when the buffer position the event pointer
 * points to will be NULL again.
 *
 *
 * @see http://www.cs.colorado.edu/department/publications/reports/docs/CU-CS-1023-07.pdf
 *      accessed Aug 26, 2010
 *      for more details on the FastForward queue.
 *
 * 
 */

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <pcl.h>

#include "stream.h"
#include "buffer.h"
#include "task.h"
#include "scheduler.h"


struct stream_desc {
  task_t *task;
  stream_t *stream;
  char mode;
  char state;
  unsigned long counter;
  struct stream_desc *next;   /* for organizing in lists */
  struct stream_desc *dirty;  /* for maintaining a list of 'dirty' elements */
};

struct stream_iter {
  stream_desc_t    *cur;
  stream_desc_t    *prev;
  stream_list_t  *list;
};



#define STDESC_OPEN     'O'
#define STDESC_CLOSED   'C'
#define STDESC_REPLACED 'R'

#define DIRTY_END   ((stream_desc_t *)-1)



static inline void MarkDirty( stream_desc_t *sd)
{

  /* only add if not dirty yet */
  if (sd->dirty == NULL) {
    /*
     * Set the dirty ptr of sd to the dirty_list ptr of the task
     * and the dirty_list ptr of the task to sd, i.e.,
     * insert the sd at the front of the dirty_list.
     * Initially, dirty_list of the tab is empty DIRTY_END (!= NULL)
     */
    sd->dirty = sd->task->dirty_list;
    sd->task->dirty_list = sd;
  }
}


/**
 * Create a stream
 */
stream_t *StreamCreate(void)
{
  stream_t *s = (stream_t *) malloc( sizeof( stream_t));
  BufferReset( &s->buffer);
  pthread_spin_init( &s->prod_lock, PTHREAD_PROCESS_PRIVATE);
  atomic_set( &s->n_sem, 0);
  atomic_set( &s->e_sem, STREAM_BUFFER_SIZE);
  s->is_poll = 0;
  s->prod_sd = NULL;
  s->cons_sd = NULL;
  return s;
}


/**
 * Destroy a stream
 */
void StreamDestroy( stream_t *s)
{
  pthread_spin_destroy( &s->prod_lock);
  free( s);
}


/**
  * Open a stream for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 * @param mode  either 'r' for reading or 'w' for writing
 */
stream_desc_t *StreamOpen( task_t *ct, stream_t *s, char mode)
{
  stream_desc_t *sd;

  assert( mode == 'r' || mode == 'w' );

  sd = (stream_desc_t *) malloc( sizeof( stream_desc_t));
  sd->task = ct;
  sd->stream = s;
  sd->mode = mode;
  sd->state = STDESC_OPEN;
  sd->counter = 0;
  sd->next  = NULL;
  sd->dirty = NULL;
  MarkDirty( sd);
  
  switch(mode) {
    case 'r': s->cons_sd = sd; break;
    case 'w': s->prod_sd = sd; break;
  }
  
  return sd;
}

/**
 * Close a stream previously opened for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 */
void StreamClose( stream_desc_t *sd, bool destroy_s)
{
  sd->state = STDESC_CLOSED;
  /* mark dirty */
  MarkDirty( sd);

  if (destroy_s) {
    StreamDestroy( sd->stream);
  }
  /* do not free sd, as it will be kept until its state
     has been output via dirty list */
}


/**
 * Replace a stream opened for reading by another stream
 * Destroys old stream.
 * @pre snew must not be opened by same or other task
 */
void StreamReplace( stream_desc_t *sd, stream_t *snew)
{
  assert( sd->mode == 'r');
  /* destroy old stream */
  StreamDestroy( sd->stream);
  /* assign new stream */
  sd->stream = snew;
  /* new consumer sd of stream */
  sd->stream->cons_sd = sd;

  sd->state = STDESC_REPLACED;
  /* counter is not reset */
  MarkDirty( sd);
}


/**
 * Non-blocking, non-consuming read from a stream
 *
 * @return    NULL if stream is empty
 */
void *StreamPeek( stream_desc_t *sd)
{ 
  assert( sd->mode == 'r');
  return BufferTop( &sd->stream->buffer);
}    


/**
 * Blocking, consuming read from a stream
 *
 * @pre       current task is single reader
 */
void *StreamRead( stream_desc_t *sd)
{
  void *item;
  task_t *self = sd->task;

  assert( sd->mode == 'r');

  /* quasi P(n_sem) */
  if ( fetch_and_dec( &sd->stream->n_sem) == 0) {
    /* wait on write */
    self->state = TASK_WAITING;
    self->wait_on = WAIT_ON_WRITE;
    self->wait_s = sd->stream;
    /* context switch */
    co_resume();
  }


  /* read the top element */
  item = BufferTop( &sd->stream->buffer);
  assert( item != NULL);
  /* pop off the top element */
  BufferPop( &sd->stream->buffer);


  /* quasi V(e_sem) */
  if ( fetch_and_inc( &sd->stream->e_sem) < 0) {
    /* e_sem was -1 */
    /* wakeup producer: make ready */
    SchedWakeup( self, sd->stream->prod_sd->task);
  }

  /* for monitoring */
  sd->counter++;
  /* mark dirty */
  MarkDirty( sd);

  return item;
}



/**
 * Blocking write to a stream
 *
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 */
void StreamWrite( stream_desc_t *sd, void *item)
{
  task_t *self = sd->task;
  int poll_wakeup = 0;

  /* check if opened for writing */
  assert( sd->mode == 'w' );
  assert( item != NULL );

  /* quasi P(e_sem) */
  if ( fetch_and_dec( &sd->stream->e_sem) == 0) {
    /* wait on write */
    self->state = TASK_WAITING;
    self->wait_on = WAIT_ON_READ;
    self->wait_s = sd->stream;
    /* context switch */
    co_resume();
  }

  /* writing to the buffer and checking if consumer polls must be atomic */
  pthread_spin_lock( &sd->stream->prod_lock);
  {
    /* there must be space now in buffer */
    assert( BufferIsSpace( &sd->stream->buffer) );
    /* put item into buffer */
    BufferPut( &sd->stream->buffer, item);

    if ( sd->stream->is_poll) {
      /* get consumer's poll token */
      poll_wakeup = atomic_swap( &sd->stream->cons_sd->task->poll_token, 0);
      sd->stream->is_poll = 0;
    }
  }
  pthread_spin_unlock( &sd->stream->prod_lock);

  /* we are the sole producer task waking the polling consumer up */
  if (poll_wakeup) {
    sd->stream->cons_sd->task->wakeup_sd = sd->stream->cons_sd;
    SchedWakeup( self, sd->stream->cons_sd->task);
  }


  /* quasi V(n_sem) */
  if ( fetch_and_inc( &sd->stream->n_sem) < 0) {
    /* n_sem was -1 */
    /* wakeup consumer: make ready */
    SchedWakeup( self, sd->stream->cons_sd->task);
  }

  /* for monitoring */
  sd->counter++;
  /* mark dirty */
  MarkDirty( sd);
}



/**
 * @pre   if file != NULL, it must be open for writing
 */
int StreamPrintDirty( task_t *t, FILE *file)
{
  stream_desc_t *sd, *next;
  int close_cnt = 0;

  assert( t != NULL);
  sd = t->dirty_list;

  if (file!=NULL) {
    fprintf( file,"[" );
  }

  while (sd != DIRTY_END) {
    /* all elements in the dirty list must belong to same task */
    assert( sd->task == t );

    /* print sd */
    if (file!=NULL) {
      fprintf( file,
          "%p,%c,%c,%lu;",
          sd->stream, sd->mode, sd->state, sd->counter
          );
    }

    /* get the next dirty entry, and clear the link in the current entry */
    next = sd->dirty;
    
    /* update states */
    if (sd->state == STDESC_REPLACED) { 
      sd->state = STDESC_OPEN;
    }
    if (sd->state == STDESC_CLOSED) {
      close_cnt++;
      free( sd);
    } else {
      sd->dirty = NULL;
    }
    sd = next;
  }
  if (file!=NULL) {
    fprintf( file,"] " );
  }
  /* */
  t->dirty_list = DIRTY_END;
  return close_cnt;
}


/****************************************************************************/
/*  Functions to support polling on a stream descriptor list                */
/****************************************************************************/


/**
 * Poll a list of streams
 *
 * @pre list must not be empty (*list != NULL)
 */
void StreamPoll( stream_list_t *list)
{
  task_t *self;
  stream_iter_t *iter;
  int do_ctx_switch = 1;

  assert( *list != NULL);

  /* get 'self', i.e. the task calling StreamPoll() */
  self = (*list)->task;

  /* place a poll token */
  atomic_set( &self->poll_token, 1);

  /* for each stream in the list */
  iter = StreamIterCreate( list);
  while( StreamIterHasNext( iter)) {
    stream_desc_t *sd = StreamIterNext( iter);
    /* lock stream (prod-side) */
    pthread_spin_lock( &sd->stream->prod_lock);
    { /* CS BEGIN */
      /* check if there is something in the buffer */
      if ( BufferTop( &sd->stream->buffer) != NULL) {
        /* yes, we can stop iterating through streams.
         * determine, if we have been woken up by another producer: 
         */
        int tok = atomic_swap( &self->poll_token, 0);
        /* we have not been waken yet, no need for ctx switch */
        if (tok) {
          do_ctx_switch = 0;
          self->wakeup_sd = sd;
        }

        /* unlock stream */
        pthread_spin_unlock( &sd->stream->prod_lock);
        /* exit loop */
        break;

      } else {
        /* nothing in the buffer, register stream as activator */
        sd->stream->is_poll = 1;
      }
    } /* CS END */
    /* unlock stream */
    pthread_spin_unlock( &sd->stream->prod_lock);
  } /* end for each stream */

  /* context switch */
  if (do_ctx_switch) {
    /* set task as waiting */
    self->state = TASK_WAITING;
    self->wait_on = WAIT_ON_ANY;
    co_resume();
  }

  /* unregister activators
   * - would only be necessary, if the consumer task closes the stream
   *   while the producer is in an is_poll state,
   *   as this could result in a SEGFAULT when the producer
   *   is trying to dereference sd->stream->cons_sd
   * - a consumer closes the stream if it read
   *   a terminate record or a sync record, and between reading the record
   *   and closing the stream the consumer issues no StreamPoll()
   *   and no entity writes a record on the stream after these records.
   */
  /*
  iter = StreamIterCreate( list);
  while( StreamIterHasNext( iter)) {
    stream_desc_t *sd = StreamIterNext( iter);
    pthread_spin_lock( &sd->stream->prod_lock);
    sd->stream->is_poll = 0;
    pthread_spin_unlock( &sd->stream->prod_lock);
  }
  */

  /* 'rotate' list to stream descriptor for non-empty buffer */
  *list = self->wakeup_sd;
  
  return;
}






/****************************************************************************/
/* Functions for maintaining a list of stream descriptors                   */
/****************************************************************************/




/**
 * it is NOT safe to append while iterating
 */
void StreamListAppend( stream_list_t *lst, stream_desc_t *node)
{
  if (*lst  == NULL) {
    /* list is empty */
    *lst = node;
    node->next = node; /* selfloop */
  } else { 
    /* insert stream between last element=*lst
       and first element=(*lst)->next */
    node->next = (*lst)->next;
    (*lst)->next = node;
    *lst = node;
  }
}


/**
 * 
 */
int StreamListIsEmpty( stream_list_t *lst)
{
  return (*lst == NULL);
}


/**
 * Creates a stream list iterator for
 * a given stream list, and, if the stream list is not empty,
 * initialises the iterator to point to the first element
 */
stream_iter_t *StreamIterCreate( stream_list_t *lst)
{
  stream_iter_t *iter = (stream_iter_t *) malloc( sizeof(stream_iter_t));
  if (lst) {
    iter->prev = *lst;
    iter->list = lst;
  }
  iter->cur = NULL;
  return iter;
}

void StreamIterDestroy( stream_iter_t *iter)
{
  free(iter);
}

/**
 * Initialises the stream list iterator to point to the first element
 * of the stream list.
 *
 * @pre The stream list is not empty, i.e. *lst != NULL
 */
void StreamIterReset( stream_list_t *lst, stream_iter_t *iter)
{
  assert(*lst != NULL);
  iter->prev = *lst;
  iter->list = lst;
  iter->cur = NULL;
}


int StreamIterHasNext( stream_iter_t *iter)
{
  return (*iter->list != NULL) &&
    ( (iter->cur != *iter->list) || (iter->cur == NULL) );
}


stream_desc_t *StreamIterNext( stream_iter_t *iter)
{
  assert( StreamIterHasNext(iter) );

  if (iter->cur != NULL) {
    /* this also does account for the state after deleting */
    iter->prev = iter->cur;
    iter->cur = iter->cur->next;
  } else {
    iter->cur = iter->prev->next;
  }
  return iter->cur;
}


void StreamIterAppend( stream_iter_t *iter, stream_desc_t *node)
{
#if 0
  /* insert after cur */
  node->next = iter->cur->next;
  iter->cur->next = node;

  /* if current node was last node, update list */
  if (iter->cur == *iter->list) {
    *iter->list = node;
  }

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
#endif

  /* insert at end of list */
  node->next = (*iter->list)->next;
  (*iter->list)->next = node;

  /* if current node was first node */
  if (iter->prev == *iter->list) {
    iter->prev = node;
  }
  *iter->list = node;

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
}

/**
 * Remove the current node from list
 * @pre iter points to valid element
 * @note IterRemove() may only be called once after
 *       IterNext(), as the current node is not a valid
 *       list node anymore. 
 */
void StreamIterRemove( stream_iter_t *iter)
{
  /* handle case if there is only a single element */
  if (iter->prev == iter->cur) {
    assert( iter->prev == *iter->list );
    iter->cur->next = NULL;
    *iter->list = NULL;
  } else {
    /* remove cur */
    iter->prev->next = iter->cur->next;
    /* cur becomes invalid */
    iter->cur->next = NULL;
    /* if the first element was deleted, clear cur */
    if (*iter->list == iter->prev) {
      iter->cur = NULL;
    } else {
      /* if the last element was deleted, correct list */
      if (*iter->list == iter->cur) {
        *iter->list = iter->prev;
      }
      iter->cur = iter->prev;
    }
  }
}


