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
 * LpelStreamRead() suspends the consumer trying to read from an empty stream,
 * LpelStreamWrite() suspends the producer trying to write to a full stream,
 * and a consumer can use LpelStreamPoll() to wait for the arrival of data
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

#include "arch/atomic.h"
#include "buffer.h"

#include "task.h"
#include "worker.h"

#include "stream.h"


/**
 * A stream which is shared between a
 * (single) producer and a (single) consumer.
 */
struct lpel_stream_t {
  unsigned int uid;         /** unique sequence number */
  buffer_t buffer;          /** buffer holding the actual data */
#ifdef STREAM_POLL_SPINLOCK /** to support polling a lock is needed */
  pthread_spinlock_t prod_lock;
#else
  pthread_mutex_t prod_lock;
#endif
  int is_poll;              /** indicates if a consumer polls this stream,
                                is_poll is protected by the prod_lock */
  lpel_stream_desc_t *prod_sd;   /** points to the sd of the producer */
  lpel_stream_desc_t *cons_sd;   /** points to the sd of the consumer */
  atomic_t n_sem;           /** counter for elements in the stream */
  atomic_t e_sem;           /** counter for empty space in the stream */
};

static atomic_t stream_seq = ATOMIC_INIT(0);



/* prototype of the function marking the stream descriptor as dirty*/
static inline void MarkDirty( lpel_stream_desc_t *sd);


/**
 * Create a stream
 *
 * Allocate and initialize memory for a stream.
 *
 * @return pointer to the created stream
 */
lpel_stream_t *LpelStreamCreate(void)
{
  lpel_stream_t *s = (lpel_stream_t *) malloc( sizeof( lpel_stream_t));
  s->uid = fetch_and_inc( &stream_seq);
  _LpelBufferReset( &s->buffer);
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
void LpelStreamDestroy( lpel_stream_t *s)
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
lpel_stream_desc_t *LpelStreamOpen( lpel_task_t *ct, lpel_stream_t *s, char mode)
{
  lpel_stream_desc_t *sd;

  assert( mode == 'r' || mode == 'w' );

  sd = (lpel_stream_desc_t *) malloc( sizeof( lpel_stream_desc_t));
  sd->task = ct;
  sd->stream = s;
  sd->sid = s->uid;
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
void LpelStreamClose( lpel_stream_desc_t *sd, int destroy_s)
{
  sd->state = STDESC_CLOSED;
  /* mark dirty */
  MarkDirty( sd);

  if (destroy_s) {
    LpelStreamDestroy( sd->stream);
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
void LpelStreamReplace( lpel_stream_desc_t *sd, lpel_stream_t *snew)
{
  assert( sd->mode == 'r');
  /* destroy old stream */
  LpelStreamDestroy( sd->stream);
  /* assign new stream */
  sd->stream = snew;
  sd->sid = snew->uid;
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
void *LpelStreamPeek( lpel_stream_desc_t *sd)
{ 
  assert( sd->mode == 'r');
  return _LpelBufferTop( &sd->stream->buffer);
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
void *LpelStreamRead( lpel_stream_desc_t *sd)
{
  void *item;
  lpel_task_t *self = sd->task;

  assert( sd->mode == 'r');

  /* quasi P(n_sem) */
  if ( fetch_and_dec( &sd->stream->n_sem) == 0) {
    /* wait on stream: */
    sd->event_flags |= STDESC_WAITON;
    MarkDirty( sd);
    _LpelTaskBlock( self, BLOCKED_ON_INPUT);
  }


  /* read the top element */
  item = _LpelBufferTop( &sd->stream->buffer);
  assert( item != NULL);
  /* pop off the top element */
  _LpelBufferPop( &sd->stream->buffer);


  /* quasi V(e_sem) */
  if ( fetch_and_inc( &sd->stream->e_sem) < 0) {
    /* e_sem was -1 */
    lpel_task_t *prod = sd->stream->prod_sd->task;
    /* wakeup producer: make ready */
    _LpelWorkerTaskWakeup( self, prod);
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
void LpelStreamWrite( lpel_stream_desc_t *sd, void *item)
{
  lpel_task_t *self = sd->task;
  int poll_wakeup = 0;

  /* check if opened for writing */
  assert( sd->mode == 'w' );
  assert( item != NULL );

  /* quasi P(e_sem) */
  if ( fetch_and_dec( &sd->stream->e_sem)== 0) {
    /* wait on stream: */
    sd->event_flags |= STDESC_WAITON;
    MarkDirty( sd);
    _LpelTaskBlock( self, BLOCKED_ON_OUTPUT);
  }

  /* writing to the buffer and checking if consumer polls must be atomic */
#ifdef STREAM_POLL_SPINLOCK
  pthread_spin_lock( &sd->stream->prod_lock);
#else
  pthread_mutex_lock( &sd->stream->prod_lock);
#endif
  {
    /* there must be space now in buffer */
    assert( _LpelBufferIsSpace( &sd->stream->buffer) );
    /* put item into buffer */
    _LpelBufferPut( &sd->stream->buffer, item);

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
    lpel_task_t *cons = sd->stream->cons_sd->task;
    /* wakeup consumer: make ready */
    _LpelWorkerTaskWakeup( self, cons);
    sd->event_flags |= STDESC_WOKEUP;
  } else {
    /* we are the sole producer task waking the polling consumer up */
    if (poll_wakeup) {
      lpel_task_t *cons = sd->stream->cons_sd->task;
      cons->wakeup_sd = sd->stream->cons_sd;
      
      _LpelWorkerTaskWakeup( self, cons);
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
 *                LpelStreamPoll() will be the one which caused the task to
 *                wakeup, i.e., the first stream where data arrived.
 */
void LpelStreamPoll( lpel_stream_list_t *list)
{
  lpel_task_t *self;
  lpel_stream_iter_t *iter;
  int do_ctx_switch = 1;

  assert( *list != NULL);

  /* get 'self', i.e. the task calling LpelStreamPoll() */
  self = (*list)->task;

  /* place a poll token */
  atomic_set( &self->poll_token, 1);

  /* for each stream in the list */
  iter = LpelStreamIterCreate( list);
  while( LpelStreamIterHasNext( iter)) {
   lpel_stream_desc_t *sd = LpelStreamIterNext( iter);
    /* lock stream (prod-side) */
#ifdef STREAM_POLL_SPINLOCK
    pthread_spin_lock( &sd->stream->prod_lock);
#else
    pthread_mutex_lock( &sd->stream->prod_lock);
#endif
    { /* CS BEGIN */
      /* check if there is something in the buffer */
      if ( _LpelBufferTop( &sd->stream->buffer) != NULL) {
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
    _LpelTaskBlock( self, BLOCKED_ON_ANYIN);
  }

  /* unregister activators
   * - would only be necessary, if the consumer task closes the stream
   *   while the producer is in an is_poll state,
   *   as this could result in a SEGFAULT when the producer
   *   is trying to dereference sd->stream->cons_sd
   * - a consumer closes the stream if it reads
   *   a terminate record or a sync record, and between reading the record
   *   and closing the stream the consumer issues no LpelStreamPoll()
   *   and no entity writes a record on the stream after these records.
   */
  /*
  iter = LpelStreamIterCreate( list);
  while( LpelStreamIterHasNext( iter)) {
   lpel_stream_desc_t *sd = LpelStreamIterNext( iter);
    pthread_spin_lock( &sd->stream->prod_lock);
    sd->stream->is_poll = 0;
    pthread_spin_unlock( &sd->stream->prod_lock);
  }
  */

  /* 'rotate' list to stream descriptor for non-empty buffer */
  *list = self->wakeup_sd;
}



/******************************************************************************/
/* HIDDEN FUNCTIONS                                                           */
/******************************************************************************/


/**
 * Reset the list of dirty stream descriptors, and call a callback function for
 * each stream descriptor.
 * 
 * @param t         the task of which the dirty list is to be printed
 * @param callback  the callback function which is called for each stream descriptor.
 *                  If it is NULL, only the dirty list is resetted.
 * @param arg       additional parameter passed to the callback function
 */
int _LpelStreamResetDirty( lpel_task_t *t,
    void (*callback)( lpel_stream_desc_t *,void*), void *arg)
{
 lpel_stream_desc_t *sd, *next;
  int close_cnt = 0;

  assert( t != NULL);
  sd = t->dirty_list;

  while (sd != STDESC_DIRTY_END) {
    /* all elements in the dirty list must belong to same task */
    assert( sd->task == t );

    /* callback function */
    if (callback) {
      callback( sd, arg);
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
  
  /* dirty list of task is empty */
  t->dirty_list = STDESC_DIRTY_END;
  return close_cnt;
}


/******************************************************************************/
/* PRIVATE FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Add a stream descriptor to the dirty list of its task.
 *
 * A stream descriptor is only added to the dirty list once.
 *
 * @param sd  stream descriptor to be marked as dirty
 */
static inline void MarkDirty( lpel_stream_desc_t *sd)
{
  lpel_task_t *t = sd->task;
  /*
   * only if task wants to collect stream info and
   * only add if not dirty yet
   */
  if ( TASK_FLAGS(t, LPEL_TASK_ATTR_MONITOR_STREAMS)
      && (sd->dirty == NULL) ) {
    /*
     * Set the dirty ptr of sd to the dirty_list ptr of the task
     * and the dirty_list ptr of the task to sd, i.e.,
     * insert the sd at the front of the dirty_list.
     * Initially, dirty_list of the tab is empty STDESC_DIRTY_END (!= NULL)
     */
    sd->dirty =t->dirty_list;
    t->dirty_list = sd;
  }
}


