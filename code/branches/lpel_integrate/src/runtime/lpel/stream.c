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

#include "stream.h"

#include "lpel.h"
#include "task.h"
/*XXX
#include "flagtree.h"
*/
#include "sysdep.h"


/**
 * Create a stream
 */
stream_t *StreamCreate(void)
{
  stream_t *s = (stream_t *) malloc( sizeof(stream_t) );
  if (s != NULL) {
    s->pread = 0;
    s->pwrite = 0;
    /* clear all the buffer space */
    memset(&(s->buf), 0, STREAM_BUFFER_SIZE*sizeof(void *));

    /* producer/consumer not assigned */
    s->prod.task = s->cons.task = NULL;
    s->prod.tbe = s->cons.tbe = NULL;
    SpinlockInit(&s->lock);
    /*XXX
    s->wany_idx = -1;
    */
    /* refcnt reflects the number of tasks
       + the stream itself opened this stream {1,2,3} */
    atomic_set(&s->refcnt, 1);
  }
  return s;
}


/**
 * Destroy a stream
 */
void StreamDestroy(stream_t *s)
{
  /* if ( fetch_and_dec(&s->refcnt) == 1 ) { */
  if ( atomic_dec(&s->refcnt) == 0 ) {
    SpinlockCleanup(&s->lock);
    free(s);
  }
}


/**
 * Open a stream for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 * @param mode  either 'r' for reading or 'w' for writing
 */
bool StreamOpen(task_t *ct, stream_t *s, char mode)
{
  /* increment reference counter of stream */
  atomic_inc(&s->refcnt);

  switch(mode) {
  case 'w':
    assert( s->prod.task == NULL );
    s->prod.task = ct;
    s->prod.tbe  = StreamtabAdd(&ct->streams_write, s, NULL);
    break;

  case 'r':
      /*XXX 
      int grpidx;
      */
    assert( s->cons.task == NULL );
    SpinlockLock(&s->lock);
    s->cons.task = ct;
    SpinlockUnlock(&s->lock);
    s->cons.tbe  = StreamtabAdd(&ct->streams_read, s, NULL);

    if ( TASK_IS_WAITANY(ct) && (s->buf[s->pread]!= NULL) ) {
      /* the consumer does set the waitany_flag itself */
      ct->waitany_flag = 1;
    }
      /*XXX */
#if 0
      s->cons.tbe  = StreamtabAdd(&ct->streams_read, s, &grpidx);
      /*if consumer task is a collector, register flagtree */
      if ( TASK_IS_WAITANY(ct) ) {
        if (grpidx > ct->waitany_info->max_grp_idx) {
          FlagtreeGrow(&ct->waitany_info->flagtree);
          ct->waitany_info->max_grp_idx *= 2;
        }
        s->wany_idx = grpidx;
        /* if stream not empty, mark flagtree */
        if (s->buf[s->pread] != NULL) {
          FlagtreeMark(&ct->waitany_info->flagtree, s->wany_idx, -1);
        }
      }
#endif
    break;

  default:
    return false;
  }
  return true;
}

/**
 * Close a stream previously opened for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 */
void StreamClose(task_t *ct, stream_t *s)
{
  if ( ct == s->prod.task ) {
    StreamtabRemove( &ct->streams_write, s->prod.tbe );
    s->prod.task = NULL;
    s->prod.tbe = NULL;

  } else if ( ct == s->cons.task ) {
    SpinlockLock(&s->lock);
    s->cons.task = NULL;
    SpinlockUnlock(&s->lock);

    StreamtabRemove( &ct->streams_read, s->cons.tbe );
    s->cons.tbe = NULL;
      
      /*XXX if consumer was collector, unregister flagtree */
#if 0
      if ( TASK_IS_WAITANY(ct) ) {
        s->wany_idx = -1;
      }
#endif
  
  } else {
    /* task is neither producer nor consumer:
       something went wrong */
    assert(0);
  }
  
  /* destroy request */
  StreamDestroy(s);
}


/**
 * Replace a stream opened for reading by another stream
 *
 * This is like calling StreamClose(s); StreamOpen(snew, 'r');
 * sequentially, but with the difference that the "position"
 * of the old stream is used for the new stream.
 * Also, the pointer to the old stream is set to point to the
 * new stream.
 *
 * @param ct    current task
 * @param s     adr of ptr to stream which to replace
 * @param snew  ptr to stream that replaces s
 */
void StreamReplace(task_t *ct, stream_t **s, stream_t *snew)
{
  /* only streams opened for reading can be replaced */
  assert( ct == (*s)->cons.task );
  /* new stream must have been closed by previous consumer */
  assert( snew->cons.task == NULL );

  /* Task has opened s, hence s contains a reference to the tbe.
     In that tbe, the stream must be replaced. Then, the reference
     to the tbe must be copied to the new stream */
  StreamtabReplace( &ct->streams_read, (*s)->cons.tbe, snew );
  snew->cons.tbe = (*s)->cons.tbe;

  SpinlockLock(&snew->lock);
  {
    /* also, set the task in the new stream */
    snew->cons.task = ct; /* ct == (*s)->cons.task */
  }
  SpinlockUnlock(&snew->lock);

  /* if new consumer is collector, and stream is not empty,
     set waitany_flag */
  if ( TASK_IS_WAITANY(ct) && snew->buf[(*s)->pread]!=NULL) {
    ct->waitany_flag = 1;
  }

  /*XXX */
#if 0
  /*if consumer is collector, register flagtree for new stream */
  snew->wany_idx = (*s)->wany_idx;
  /* if stream not empty, mark flagtree */
  if (snew->wany_idx >= 0 && snew->buf[(*s)->pread] != NULL) {
    FlagtreeMark(&ct->waitany_info->flagtree, snew->wany_idx, -1);
  }
#endif

  /* NOTE: both producers of s and snew do possibly mark the flagtree now,
   *  until the following CS is executed - has this to be considered harmful?
   */

  /* clear references in old stream */
  SpinlockLock(&(*s)->lock);
  {
    (*s)->cons.task = NULL;
  }
  SpinlockUnlock(&(*s)->lock);
  (*s)->cons.tbe = NULL;
  /*XXX
    (*s)->wany_idx = -1;
   */

  /* destroy request for old stream */
  StreamDestroy(*s);

  /* Let s point to the new stream */
  *s = snew;
}


/**
 * Non-blocking read from a stream
 *
 * @param ct  pointer to current task
 * @pre       current task is single reader
 * @param s   stream to read from
 * @return    NULL if stream is empty
 */
void *StreamPeek(task_t *ct, stream_t *s)
{ 
  /* check if opened for reading */
  assert( s->cons.task == ct );

  /* if the buffer is empty, buf[pread]==NULL */
  return s->buf[s->pread];  
}    


/**
 * Blocking read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 *
 * @param ct  pointer to current task
 * @param s   stream to read from
 * @pre       current task is single reader
 */
void *StreamRead(task_t *ct, stream_t *s)
{
  void *item;

  /* check if opened for reading */
  assert( s->cons.task == ct );

  /* wait if buffer is empty */
  if ( s->buf[s->pread] == NULL ) {
    TaskWaitOnWrite(ct, s);
    assert( s->buf[s->pread] != NULL );
  }

  /* READ FROM BUFFER */
  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= STREAM_BUFFER_SIZE) ?
              (1-STREAM_BUFFER_SIZE) : 1;

  /* for monitoring */
  if (ct!=NULL) StreamtabEvent( &ct->streams_read, s->cons.tbe );

  return item;
}


/**
 * Check if there is space in the buffer
 *
 * A writer can use this function before a write
 * to ensure the write succeeds (without blocking)
 *
 * @param ct  pointer to current task
 * @param s   stream opened for writing
 * @pre       current task is single writer
 */
bool StreamIsSpace(task_t *ct, stream_t *s)
{
  /* check if opened for writing */
  assert( s->prod.task == ct );

  /* if there is space in the buffer, the location at pwrite holds NULL */
  return ( s->buf[s->pwrite] == NULL );
}


/**
 * Blocking write to a stream
 *
 * Precondition: item != NULL
 *
 * Implementation note:
 * - modifies only pwrite pointer (not pread)
 *
 * @param ct    pointer to current task
 * @param s     stream to write to
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 */
void StreamWrite(task_t *ct, stream_t *s, void *item)
{
  /* check if opened for writing */
  assert( s->prod.task == ct );

  assert( item != NULL );

  /* wait while buffer is full */
  if ( s->buf[s->pwrite] != NULL ) {
    TaskWaitOnRead(ct, s);
    assert( s->buf[s->pwrite] == NULL );
  }

  /* WRITE TO BUFFER */
  /* Write Memory Barrier: ensure all previous memory write 
   * are visible to the other processors before any later
   * writes are executed.  This is an "expensive" memory fence
   * operation needed in all the architectures with a weak-ordering 
   * memory model where stores can be executed out-or-order 
   * (e.g. PowerPC). This is a no-op on Intel x86/x86-64 CPUs.
   */
  WMB(); 
  s->buf[s->pwrite] = item;
  s->pwrite += (s->pwrite+1 >= STREAM_BUFFER_SIZE) ?
               (1-STREAM_BUFFER_SIZE) : 1;

  /* for monitoring */
  if (ct!=NULL) StreamtabEvent( &ct->streams_write, s->prod.tbe );

  SpinlockLock(&s->lock);

  if ( s->cons.task!=NULL && TASK_IS_WAITANY(s->cons.task)) {
    s->cons.task->waitany_flag = 1;
  }
  /*XXX  if flagtree registered, use flagtree mark
  if (s->wany_idx >= 0) {
    FlagtreeMark(
        &s->cons.task->waitany_info->flagtree,
        s->wany_idx,
        ct->owner
        );
  }
  */
  SpinlockUnlock(&s->lock);
}


#if 0

/**
 *TODO
 */
void StreamWaitAny(task_t *ct)
{
  assert( TASK_IS_WAITANY(ct) );
  TaskWaitOnAny(ct);
  StreamtabIterateStart(
      &ct->streams_read,
      &ct->waitany_info->iter
      );
}

/**
 *TODO
 */
stream_t *StreamIterNext(task_t *ct)
{
  assert( TASK_IS_WAITANY(ct) );
  streamtbe_t *ste = StreamtabIterateNext(
      &ct->streams_read,
      &ct->waitany_info->iter
      );
  return ste->s;
}

/**
 *TODO
 */
bool StreamIterHasNext(task_t *ct)
{
  assert( TASK_IS_WAITANY(ct) );
  return StreamtabIterateHasNext(
      &ct->streams_read,
      &ct->waitany_info->iter
      ) > 0;
}
#endif

