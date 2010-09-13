/* 
 * Buffer, implemented as Single-Writer Single-Reader circular buffer.
 *
 * It uses ideas from the FastForward queue implementation:
 * Synchronisation takes place with the content of the buffer, i.e., NULL indicates that
 * the location is empty. After reading a value, the consumer writes NULL back to the
 * position it read the data from.
 *
 * There is the possibility to register a pointer to a flag which will be set upon writing
 * to the buffer. 
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


#include "buffer.h"
#include "sysdep.h"

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing between core's private cache */
struct buffer {
  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  void *data[STREAM_BUFFER_SIZE];
  pthread_spinlock_t lock;
  volatile void **flag_ptr;
  volatile void *flag_stub;
};



/**
 * Create a buffer
 */
buffer_t *BufferCreate(void)
{
  buffer_t *buf = (buffer_t *) malloc( sizeof(buffer_t) );
  if (buf != NULL) {
    buf->pread = 0;
    buf->pwrite = 0;
    /* clear all the buffer space */
    memset(&(buf->data), 0, STREAM_BUFFER_SIZE*sizeof(void *));

    buf->flag_stub = (void *)0x0;
    buf->flag_ptr = &buf->flag_stub;
    pthread_spin_init( &buf->lock, 0);
  }
  return buf;
}


/**
 * Destroy a buffer
 */
void BufferDestroy( buffer_t *buf)
{
  pthread_spin_destroy( &buf->lock);
  free(buf);
}



/**
 * Returns the top from a buffer
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 * @return      NULL if buffer is empty
 */
void *BufferTop( buffer_t *buf)
{ 
  /* if the buffer is empty, data[pread]==NULL */
  return buf->data[buf->pread];  
}    


/**
 * Consuming read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 */
void BufferPop( buffer_t *buf)
{
  /* clear, and advance pread */
  buf->data[buf->pread]=NULL;
  buf->pread += (buf->pread+1 >= STREAM_BUFFER_SIZE) ?
              (1-STREAM_BUFFER_SIZE) : 1;
}


/**
 * Check if there is space in the buffer
 *
 * @param buf   buffer to check
 * @pre         no concurrent calls
 */
int BufferIsSpace( buffer_t *buf)
{
  /* if there is space in the buffer, the location at pwrite holds NULL */
  return ( buf->data[buf->pwrite] == NULL );
}


/**
 * Put an item into the buffer
 *
 * Precondition: item != NULL
 *
 * Implementation note:
 * - modifies only pwrite pointer (not pread)
 *
 * @param buf   buffer to write to
 * @param item  data item (a pointer) to write
 * @pre         no concurrent writes
 * @pre         item != NULL
 * @pre         there has to be space in the buffer
 *              (check with BufferIsSpace)
 */
void BufferPut( buffer_t *buf, void *item)
{
  assert( item != NULL );
  assert( BufferIsSpace(buf) );

  /* WRITE TO BUFFER */
  /* Write Memory Barrier: ensure all previous memory write 
   * are visible to the other processors before any later
   * writes are executed.  This is an "expensive" memory fence
   * operation needed in all the architectures with a weak-ordering 
   * memory model where stores can be executed out-or-order 
   * (e.g. PowerPC). This is a no-op on Intel x86/x86-64 CPUs.
   */
  WMB(); 
  buf->data[buf->pwrite] = item;
  buf->pwrite += (buf->pwrite+1 >= STREAM_BUFFER_SIZE) ?
               (1-STREAM_BUFFER_SIZE) : 1;

  pthread_spin_lock( &buf->lock);
  *buf->flag_ptr = (void *)0x1;
  pthread_spin_unlock( &buf->lock);
}


/**
 * @pre only the writer calls this function
 */
void **BufferGetWptr( buffer_t *buf)
{
  return &buf->data[buf->pwrite];
}


/**
 * @pre only the reader calls this function
 */
void **BufferGetRptr( buffer_t *buf)
{
  return &buf->data[buf->pread];
}

void BufferRegisterFlag( buffer_t *buf, volatile void **flag_ptr)
{
  pthread_spin_lock( &buf->lock);
  buf->flag_ptr = ( (void**)flag_ptr != NULL) ? flag_ptr : &buf->flag_stub;
  *buf->flag_ptr = (void *)0x1;
  pthread_spin_unlock( &buf->lock);
}

