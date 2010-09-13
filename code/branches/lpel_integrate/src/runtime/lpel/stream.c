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
#include "buffer.h"
#include "task.h"

struct stream_mh {
  task_t *task;
  stream_t *stream;
  char mode;
  char state;
  unsigned long counter;
  struct stream_mh *dirty;
};

#define STMH_OPEN     'O'
#define STMH_CLOSED   'C'
#define STMH_REPLACED 'R'

#define DIRTY_END   ((stream_mh_t *)-1)



static inline void MarkDirty( stream_mh_t *mh)
{

  /* only add if not dirty yet */
  if (mh->dirty == NULL) {
    /*
     * Set the dirty ptr of mh to the dirty_list ptr of the task
     * and the dirty_list ptr of the task to mh, i.e.,
     * insert the mh at the front of the dirty_list.
     * Initially, dirty_list of the tab is empty DIRTY_END (!= NULL)
     */
    stream_mh_t **dirty_list = TaskGetDirtyStreams( mh->task);
    mh->dirty = *dirty_list;
    *dirty_list = mh;
  }
}


/**
 * Create a stream
 */
stream_t *StreamCreate(void)
{
  return BufferCreate();
}




/**
  * Open a stream for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 * @param mode  either 'r' for reading or 'w' for writing
 */
stream_mh_t *StreamOpen( task_t *ct, stream_t *s, char mode)
{
  stream_mh_t *mh = (stream_mh_t *) malloc( sizeof( stream_mh_t));

  switch(mode) {
  case 'r':
    BufferRegisterFlag( s, &ct->wany_flag);
  case 'w':
    mh->task = ct;
    mh->stream = s;
    mh->mode = mode;
    mh->state = STMH_OPEN;
    mh->counter = 0;
    mh->dirty = NULL;
    MarkDirty( mh);
    break;

  default:
    free( mh);
    return NULL;
  }
  return mh;
}

/**
 * Close a stream previously opened for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 */
void StreamClose( stream_mh_t *mh, bool destroy_s)
{
  mh->state = STMH_CLOSED;
  /* mark dirty */
  //TODO mh->task;

  if (mh->mode=='r') {
    BufferRegisterFlag( mh->stream, NULL);
  }
  if (destroy_s) {
    BufferDestroy( mh->stream);
  }
  /* do not free mh, as it will be kept until its state
     has been output via dirty list */
}


/**
 * Replace a stream opened for reading by another stream
 * Destroys old stream.
 * @pre snew must not be opened by same or other task
 */
void StreamReplace( stream_mh_t *mh, stream_t *snew)
{
  assert( mh->mode == 'r');
  /* destroy old stream */
  BufferDestroy( mh->stream);
  /* assign new stream and register flag */
  mh->stream = snew;
  BufferRegisterFlag( mh->stream, &mh->task->wany_flag);
  mh->state = STMH_REPLACED;
  /* counter is not reset */
  MarkDirty( mh);
}


/**
 * Non-blocking, non-consuming read from a stream
 *
 * @return    NULL if stream is empty
 */
void *StreamPeek( stream_mh_t *mh)
{ 
  assert( mh->mode == 'r');
  return BufferTop( mh->stream);
}    


/**
 * Blocking, consuming read from a stream
 *
 * @pre       current task is single reader
 */
void *StreamRead( stream_mh_t *mh)
{
  void *item;

  assert( mh->mode == 'r');
  
  item = BufferTop( mh->stream);
  /* wait if buffer is empty */
  if ( item == NULL ) {
    TaskWaitOnWrite( mh->task, mh->stream);
    item = BufferTop( mh->stream);
  }
  assert( item != NULL);
  /* pop off the top element */
  BufferPop( mh->stream);

  /* for monitoring */
  mh->counter++;
  /* mark dirty */
  MarkDirty( mh);

  return item;
}



/**
 * Blocking write to a stream
 *
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 */
void StreamWrite( stream_mh_t *mh, void *item)
{
  /* check if opened for writing */
  assert( mh->mode == 'w' );
  assert( item != NULL );

  /* wait while buffer is full */
  if ( !BufferIsSpace( mh->stream) ) {
    TaskWaitOnRead( mh->task, mh->stream);
  }
  assert( BufferIsSpace( mh->stream) );
  /* put item into buffer */
  BufferPut( mh->stream, item);

  /* for monitoring */
  mh->counter++;
  /* mark dirty */
  MarkDirty( mh);
}



/**
 * @pre   if file != NULL, it must be open for writing
 */
int StreamPrintDirty( task_t *t, FILE *file)
{
  stream_mh_t *mh, *next;
  int close_cnt = 0;

  assert( t != NULL);
  mh = *TaskGetDirtyStreams( t);

  if (file!=NULL) {
    fprintf( file,"[" );
  }

  while (mh != DIRTY_END) {
    /* all elements in the dirty list must belong to same task */
    assert( mh->task == t );

    /* print mh */
    if (file!=NULL) {
      fprintf( file,
          "%p,%c,%c,%lu;",
          mh->stream, mh->mode, mh->state, mh->counter
          );
    }

    /* get the next dirty entry, and clear the link in the current entry */
    next = mh->dirty;
    
    /* update states */
    if (mh->state == STMH_REPLACED) { mh->state = STMH_OPEN; }
    if (mh->state == STMH_CLOSED) {
      close_cnt++;
      free( mh);
    } else {
      mh->dirty = NULL;
    }
    mh = next;
  }
  if (file!=NULL) {
    fprintf( file,"] " );
  }
  /* */
  *(TaskGetDirtyStreams(t)) = DIRTY_END;
  return close_cnt;
}
