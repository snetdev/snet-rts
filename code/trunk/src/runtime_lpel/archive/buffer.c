/* 
 * This implements a buffer.
 */


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <buffer.h>
#include <bool.h>
#include <memfun.h>
#include <record.h>
#include <pthread.h>
#include "debug.h"

#include "extended_semaphore.h"

#define BUFFER_DEBUG

struct buffer  {
  void **ringBuffer;
  unsigned int bufferCapacity;
  unsigned int bufferSpaceLeft;
  unsigned int bufferHead;
  unsigned int bufferEnd;

  pthread_mutex_t *mxCounter;

  snet_ex_sem_t *record_count;
};

bool SNetBufIsEmpty(snet_buffer_t *buf) {
  int lock_status;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %x) (SNetBufIsEmpty (buf %x)) "
                       "(Buffer is not locked.))",
                       (unsigned int) buf, (unsigned int) buf);
  }
  SNetUtilDebugNotice("buffer mutex: %x", (unsigned int) buf->mxCounter);
  return SNetExSemIsMinimal(buf->record_count);
}

snet_buffer_t *SNetBufCreate( unsigned int size, pthread_mutex_t *lock) {
  snet_buffer_t *theBuffer;
       
  theBuffer = SNetMemAlloc( sizeof( snet_buffer_t));
  SNetUtilDebugNotice("(CREATION (BUFFER %x) ((size %d) (lock %x)))",
                      (unsigned int) theBuffer,
                      size,
                      (unsigned int) lock);

  theBuffer->ringBuffer = SNetMemAlloc( size * sizeof( void**));
  theBuffer->bufferCapacity=size;
  theBuffer->bufferHead=0;
  theBuffer->bufferEnd=0;
  theBuffer->bufferSpaceLeft=size;

  theBuffer->mxCounter = lock;
  theBuffer->record_count = SNetExSemCreate(lock, 
                                            true, 0,
                                            true, size,
                                            0);

  return( theBuffer);
}

snet_buffer_t *SNetBufPut( snet_buffer_t *bf, void* elem) {
  int lock_status;

  #ifdef BUFFER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (BUFFER %x) (SNetBufPut (buf %x) (elem %x)))",
                      (unsigned int) bf, (unsigned int) bf, (unsigned int) elem);
  #endif
  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %x) (SNetBufPut (buf %x)) (elem %x) "
                       "(Buffer is not locked))",
                       (unsigned int) bf, (unsigned int) bf, (unsigned int) elem);
  }

  bf->record_count = SNetExSemIncrement(bf->record_count);

  /* put data into the buffer */
  bf->ringBuffer[ bf->bufferHead ] = elem;  
  bf->bufferHead += 1;
  bf->bufferHead %= bf->bufferCapacity;
  bf->bufferSpaceLeft -= 1;
	
  return( bf);
}

extern void *SNetBufGet( snet_buffer_t *bf) {
  void *result;
  int lock_status;
  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %x) (SNetBufGet (buf %x)) "
                       "(Buffer is not locked))",
                       (unsigned int) bf, (unsigned int) bf);
  }

  SNetUtilDebugNotice("spaceLeft = %d, capacity = %d, semaphore value = %d",
                      bf->bufferSpaceLeft, bf->bufferCapacity, 
                      SNetExSemGetValue(bf->record_count));

  bf->record_count = SNetExSemDecrement(bf->record_count);

  /* get the data from the buffer*/
  result = (bf->ringBuffer[ bf->bufferEnd++ ]);   
  bf->bufferEnd %= bf->bufferCapacity;
  bf->bufferSpaceLeft += 1;
  
  return result;
}

extern void *SNetBufShow( snet_buffer_t *buf) {
  int lock_status;
  void *result;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  SNetUtilDebugNotice("lock_status = %d, EBUSY = %d", lock_status, EBUSY);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %x) (SNetBufShow (buf %x)) "
                       "(Buffer is not locked))", (unsigned int) buf, 
                       (unsigned int) buf);
  }

  if(SNetBufIsEmpty(buf)) {
    result = NULL;
  } else {
    result = buf->ringBuffer[buf->bufferEnd];
  }
    
  return result;
}
  
/* TODO: Rename to something like 'increase semaphore by record_count' */
extern void SNetBufRegisterDispatcher( snet_buffer_t *bf, snet_ex_sem_t *sem) {
  int i;
  int lock_status;
  int buffer_capacity;
  int space_left;
  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %x) (BufRegisterDispatcher (bf %x) (sem %x))"
                       "(Buffer not locked))");
  }

  /* TODO: make this use getvalue of the semaphore? */
  buffer_capacity = bf->bufferCapacity;
  space_left = bf->bufferSpaceLeft;
  for( i=0; i<( buffer_capacity - space_left); i++) {
    SNetExSemIncrement( sem);
  }
}

extern void SNetBufDestroy( snet_buffer_t *bf) {
  SNetMemFree( bf->ringBuffer);
  SNetExSemDestroy(bf->record_count);
  SNetMemFree( bf);
}
