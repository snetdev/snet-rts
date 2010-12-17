/**
 * File: stream.c
 * Auth: Daniel Prokesch <daniel.prokesch@gmail.com>
 * Date: 2010/08/26
 *
 * Desc:
 * 
 * Core stream handling functions, including stream descriptors,
 * stream descriptor lists and iterators.
 *
 * A stream is the communication and synchronization primitive between two
 * tasks. If a task wants to use a stream, it must open it in order to
 * retrieve a stream descriptor. A task can open it for reading ('r',
 * a consumer) or writing ('w', a producer) but not both, meaning that
 * streams are uni-directional. A single stream can be opened by at most one
 * producer task and by at most one consumer task simultaneously.
 *
 * After opening, a consumer can read from a stream and a producer can write
 * to a stream, using the retrieved stream descriptor. Note that only the
 * streams are shared, not the stream descriptors.
 *
 * Within a stream a buffer struct is holding the actual data, which is
 * implemented as circular single-producer single-consumer in a concurrent,
 * lock-free manner.
 *
 * For synchronization between tasks, three blocking functions are provided:
 * StreamRead() suspends the consumer trying to read from an empty stream,
 * StreamWrite() suspends the producer trying to write to a full stream,
 * and a consumer can use StreamPoll() to wait for the arrival of data
 * on any of the streams specified in a list.
 *
 *TODO describe stream descriptor lists, iterators
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

#include "stream.h"
#include "buffer.h"
#include "atomic.h"
#include "task.h"
#include "scheduler.h"


/**
 * A stream which is shared between a
 * (single) producer and a (single) consumer.
 */
struct stream {
  buffer_t buffer;          /** buffer holding the actual data */
#ifdef STREAM_POLL_SPINLOCK /** to support polling a lock is needed */
  pthread_spinlock_t prod_lock;
#else
  pthread_mutex_t prod_lock;
#endif
  int is_poll;              /** indicates if a consumer polls this stream,
                                is_poll is protected by the prod_lock */
  stream_desc_t *prod_sd;   /** points to the sd of the producer */
  stream_desc_t *cons_sd;   /** points to the sd of the consumer */
  atomic_t n_sem;           /** counter for elements in the stream */
  atomic_t e_sem;           /** counter for empty space in the stream */
};


/**
 * A stream descriptor
 *
 * A producer/consumer must open a stream before using it, and by opening
 * a stream, a stream descriptor is created and returned. 
 */
struct stream_desc {
  task_t *task;       /** the task which opened the stream */
  stream_t *stream;   /** pointer to the stream */
  char mode;          /** either 'r' or 'w' */
  char state;         /** one of IOCR, for monitoring */
  unsigned long counter;      /** counts the number of items transmitted
                                  over the stream descriptor */
  int event_flags;            /** which events happened on that stream */
  struct stream_desc *next;   /** for organizing in lists */
  struct stream_desc *dirty;  /** for maintaining a list of 'dirty' items */
};


/**
 * An iterator for a stream descriptor list
 */
struct stream_iter {
  stream_desc_t *cur;
  stream_desc_t *prev;
  stream_list_t *list;
};


/**
 * The state of a stream descriptor
 */
#define STDESC_INUSE    'I'
#define STDESC_OPENED   'O'
#define STDESC_CLOSED   'C'
#define STDESC_REPLACED 'R'

/**
 * The event_flags of a stream descriptor
 */
#define STDESC_MOVED    (1<<0)
#define STDESC_WOKEUP   (1<<1)
#define STDESC_WAITON   (1<<2)

/**
 * This special value indicates the end of the dirty list chain.
 * NULL cannot be used as NULL indicates that the SD is not dirty.
 */
#define DIRTY_END   ((stream_desc_t *)-1)


/* prototype of the function marking the stream descriptor as dirty*/
static inline void MarkDirty( stream_desc_t *sd);


/**
 * Create a stream
 *
 * Allocate and initialize memory for a stream.
 *
 * @return pointer to the created stream
 */
stream_t *StreamCreate(void)
{
  stream_t *s = (stream_t *) malloc( sizeof( stream_t));
  BufferReset( &s->buffer);
#ifdef STREAM_POLL_SPINLOCK
  pthread_spin_init( &s->prod_lock, PTHREAD_PROCESS_PRIVATE);
#else
  pthread_mutex_init( &s->prod_lock, NULL);
#endif
  atomic_init( &s->n_sem, 0);
  atomic_init( &s->e_sem, STREAM_BUFFER_SIZE);
  s->is_poll = 0;
  s->prod_sd = NULL;
  s->cons_sd = NULL;
  return s;
}


/**
 * Destroy a stream
 *
 * Free the memory allocated for a stream.
 *
 * @param s   stream to be freed
 * @pre       stream must not be opened by any task!
 */
void StreamDestroy( stream_t *s)
{
#ifdef STREAM_POLL_SPINLOCK
  pthread_spin_destroy( &s->prod_lock);
#else
  pthread_mutex_destroy( &s->prod_lock);
#endif
  atomic_destroy( &s->n_sem);
  atomic_destroy( &s->e_sem);
  free( s);
}


/**
  * Open a stream for reading/writing
 *
 * @param ct    pointer to current task
 * @param s     pointer to stream
 * @param mode  either 'r' for reading or 'w' for writing
 * @return      a stream descriptor
 * @pre         only one task may open it for reading resp. writing
 *              at any given point in time
 */
stream_desc_t *StreamOpen( task_t *ct, stream_t *s, char mode)
{
  stream_desc_t *sd;

  assert( mode == 'r' || mode == 'w' );

  sd = (stream_desc_t *) malloc( sizeof( stream_desc_t));
  sd->task = ct;
  sd->stream = s;
  sd->mode = mode;
  sd->state = STDESC_OPENED;
  sd->counter = 0;
  sd->event_flags = 0;
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
 * @param sd          stream descriptor
 * @param destroy_s   if true, destroy the stream as well
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
 *
 * @param sd    stream descriptor for which the stream must be replaced
 * @param snew  the new stream
 * @pre         snew must not be opened by same or other task
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
 * @param sd  stream descriptor
 * @return    the top item of the stream, or NULL if stream is empty
 */
void *StreamPeek( stream_desc_t *sd)
{ 
  assert( sd->mode == 'r');
  return BufferTop( &sd->stream->buffer);
}    


/**
 * Blocking, consuming read from a stream
 *
 * If the stream is empty, the task is suspended until
 * a producer writes an item to the stream.
 *
 * @param sd  stream descriptor
 * @return    the next item of the stream
 * @pre       current task is single reader
 */
void *StreamRead( stream_desc_t *sd)
{
  void *item;
  task_t *self = sd->task;

  assert( sd->mode == 'r');

  /* quasi P(n_sem) */
  if ( fetch_and_dec( &sd->stream->n_sem) == 0) {
    self->state = TASK_BLOCKED;
    self->wait_on = WAIT_ON_WRITE;
    /* wait on stream: */
    sd->event_flags |= STDESC_WAITON;
    MarkDirty( sd);
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
    task_t *prod = sd->stream->prod_sd->task;
    /* wakeup producer: make ready */
    SchedWakeup( self, prod);
    sd->event_flags |= STDESC_WOKEUP;
  }

  /* for monitoring */
  sd->counter++;
  sd->event_flags |= STDESC_MOVED;
  /* mark dirty */
  MarkDirty( sd);

  return item;
}



/**
 * Blocking write to a stream
 *
 * If the stream is full, the task is suspended until the consumer
 * reads items from the stream, freeing space for more items.
 *
 * @param sd    stream descriptor
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
  if ( fetch_and_dec( &sd->stream->e_sem)== 0) {
    self->state = TASK_BLOCKED;
    self->wait_on = WAIT_ON_READ;
    /* wait on stream: */
    sd->event_flags |= STDESC_WAITON;
    MarkDirty( sd);
    /* context switch */
    co_resume();
  }

  /* writing to the buffer and checking if consumer polls must be atomic */
#ifdef STREAM_POLL_SPINLOCK
  pthread_spin_lock( &sd->stream->prod_lock);
#else
  pthread_mutex_lock( &sd->stream->prod_lock);
#endif
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
#ifdef STREAM_POLL_SPINLOCK
  pthread_spin_unlock( &sd->stream->prod_lock);
#else
  pthread_mutex_unlock( &sd->stream->prod_lock);
#endif



  /* quasi V(n_sem) */
  if ( fetch_and_inc( &sd->stream->n_sem) < 0) {
    /* n_sem was -1 */
    task_t *cons = sd->stream->cons_sd->task;
    /* wakeup consumer: make ready */
    SchedWakeup( self, cons);
    sd->event_flags |= STDESC_WOKEUP;
  } else {
    /* we are the sole producer task waking the polling consumer up */
    if (poll_wakeup) {
      task_t *cons = sd->stream->cons_sd->task;
      cons->wakeup_sd = sd->stream->cons_sd;
      
      SchedWakeup( self, cons);
      sd->event_flags |= STDESC_WOKEUP;
    }
  }

  /* for monitoring */
  sd->counter++;
  sd->event_flags |= STDESC_MOVED;
  /* mark dirty */
  MarkDirty( sd);
}



/**
 * Poll a list of streams
 *
 * This is a blocking function called by a consumer which wants to wait
 * for arrival of data on any of a specified list of streams.
 * The consumer task is suspended while there is no new data on all streams.
 *
 * @param list    a stream descriptor list the task wants to poll
 * @pre           list must not be empty (*list != NULL)
 *
 * @post          The first element when iterating through the list after
 *                StreamPoll() will be the one which caused the task to
 *                wakeup, i.e., the first stream where data arrived.
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
#ifdef STREAM_POLL_SPINLOCK
    pthread_spin_lock( &sd->stream->prod_lock);
#else
    pthread_mutex_lock( &sd->stream->prod_lock);
#endif
    { /* CS BEGIN */
      /* check if there is something in the buffer */
      if ( BufferTop( &sd->stream->buffer) != NULL) {
        /* yes, we can stop iterating through streams.
         * determine, if we have been woken up by another producer: 
         */
        int tok = atomic_swap( &self->poll_token, 0);
        if (tok) {
          /* we have not been woken yet, no need for ctx switch */
          do_ctx_switch = 0;
          self->wakeup_sd = sd;
        }
        /* unlock stream */
#ifdef STREAM_POLL_SPINLOCK
        pthread_spin_unlock( &sd->stream->prod_lock);
#else
        pthread_mutex_unlock( &sd->stream->prod_lock);
#endif
        /* exit loop */
        break;

      } else {
        /* nothing in the buffer, register stream as activator */
        sd->stream->is_poll = 1;
        sd->event_flags |= STDESC_WAITON;
        /* TODO marking all streams does potentially flood the log-files
           - is it desired to have anyway?
        MarkDirty( sd);
        */
      }
    } /* CS END */
    /* unlock stream */
#ifdef STREAM_POLL_SPINLOCK
    pthread_spin_unlock( &sd->stream->prod_lock);
#else
    pthread_mutex_unlock( &sd->stream->prod_lock);
#endif
  } /* end for each stream */

  /* context switch */
  if (do_ctx_switch) {
    /* set task as blocked */
    self->state = TASK_BLOCKED;
    self->wait_on = WAIT_ON_ANY;
    co_resume();
  }

  /* unregister activators
   * - would only be necessary, if the consumer task closes the stream
   *   while the producer is in an is_poll state,
   *   as this could result in a SEGFAULT when the producer
   *   is trying to dereference sd->stream->cons_sd
   * - a consumer closes the stream if it reads
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
}



/****************************************************************************/
/*  Printing, for monitoring output                                         */
/****************************************************************************/

#define IS_FLAGS(vec,b) ( ((vec)&(b)) == (b))

/**
 * Print the list of dirty stream descriptors of a task into a file.
 *
 * After printing, the dirty list is empty again and the state of each
 * stream descriptor is updated/resetted.
 * 
 * @param t     the task of which the dirty list is to be printed
 * @param file  the file to which the dirty list should be printed,
 *              or NULL if only the dirty list should be cleared
 * @pre         if file != NULL, it must be open for writing
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
          "%p,%c,%c,%lu,%c%c%c;",
          sd->stream, sd->mode, sd->state, sd->counter,
          IS_FLAGS( sd->event_flags, STDESC_WAITON) ? '?':'-',
          IS_FLAGS( sd->event_flags, STDESC_WOKEUP) ? '!':'-',
          IS_FLAGS( sd->event_flags, STDESC_MOVED ) ? '*':'-'
          );
    }

    /* get the next dirty entry, and clear the link in the current entry */
    next = sd->dirty;
    
    /* update/reset states */
    switch (sd->state) {
      case STDESC_OPENED:
      case STDESC_REPLACED:
        sd->state = STDESC_INUSE;
      case STDESC_INUSE:
        sd->dirty = NULL;
        sd->event_flags = 0;
        break;
      case STDESC_CLOSED:
        close_cnt++;
        free( sd);
        break;
      default: assert(0);
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
/* Private functions                                                        */
/****************************************************************************/

/**
 * Add a stream descriptor to the dirty list of its task.
 *
 * A stream descriptor is only added to the dirty list once.
 *
 * @param sd  stream descriptor to be marked as dirty
 */
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





/****************************************************************************/
/* Functions for maintaining a list of stream descriptors                   */
/****************************************************************************/



/**
 * Append a stream descriptor to a stream descriptor list
 *
 * @param lst   the stream descriptor list
 * @param node  the stream descriptor to be appended
 *
 * @pre   it is NOT safe to append while iterating, i.e. if an SD should
 *        be appended while the list is iterated through, StreamIterAppend()
 *        must be used.
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
 * Test if a stream descriptor list is empty
 *
 * @param lst   stream descriptor list
 * @return      1 if the list is empty, 0 otherwise
 */
int StreamListIsEmpty( stream_list_t *lst)
{
  return (*lst == NULL);
}


/**
 * Create a stream iterator
 * 
 * Creates a stream descriptor iterator for a given stream descriptor list,
 * and, if the stream descriptor list is not empty, initialises the iterator
 * to point to the first element such that it is ready to be used.
 * If the list is empty, it must be resetted with StreamIterReset()
 * before usage.
 *
 * @param lst   list to create an iterator from, can be NULL to only allocate
 *              the memory for the iterator
 * @return      the newly created iterator
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

/**
 * Destroy a stream descriptor iterator
 *
 * Free the memory for the specified iterator.
 *
 * @param iter  iterator to be destroyed
 */
void StreamIterDestroy( stream_iter_t *iter)
{
  free(iter);
}

/**
 * Initialises the stream list iterator to point to the first element
 * of the stream list.
 *
 * @param lst   list to be iterated through
 * @param iter  iterator to be resetted
 * @pre         The stream list is not empty, i.e. *lst != NULL
 */
void StreamIterReset( stream_list_t *lst, stream_iter_t *iter)
{
  assert( lst != NULL);
  iter->prev = *lst;
  iter->list = lst;
  iter->cur = NULL;
}

/**
 * Test if there are more stream descriptors in the list to be
 * iterated through
 *
 * @param iter  the iterator
 * @return      1 if there are stream descriptors left, 0 otherwise
 */
int StreamIterHasNext( stream_iter_t *iter)
{
  return (*iter->list != NULL) &&
    ( (iter->cur != *iter->list) || (iter->cur == NULL) );
}


/**
 * Get the next stream descriptor from the iterator
 *
 * @param iter  the iterator
 * @return      the next stream descriptor
 * @pre         there must be stream descriptors left for iteration,
 *              check with StreamIterHasNext()
 */
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

/**
 * Append a stream descriptor to a list while iterating
 *
 * Appends the SD at the end of the list.
 * [Uncomment the source to insert the specified SD
 * after the current SD instead.]
 *
 * @param iter  iterator for the list currently in use
 * @param node  stream descriptor to be appended
 */
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
#else
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
#endif
}

/**
 * Remove the current stream descriptor from list while iterating through
 *
 * @param iter  the iterator
 * @pre         iter points to valid element
 *
 * @note StreamIterRemove() may only be called once after
 *       StreamIterNext(), as the current node is not a valid
 *       list node anymore. Iteration can be continued though.
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


