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

struct buffer  {
  void **ringBuffer;
  unsigned int bufferCapacity;
  unsigned int bufferSpaceLeft;
  unsigned int bufferHead;
  unsigned int bufferEnd;

  pthread_mutex_t *mxCounter;

  snet_ex_sem_t *record_count;
  bool flag;
};

bool SNetBufIsEmpty(snet_buffer_t *buf) {
  int lock_status;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufIsEmpty (buf %p)) "
                       "(Buffer is not locked.))",
                       buf, buf);
  }

  return SNetExSemIsMinimal(buf->record_count);
}

unsigned int SNetBufSize( snet_buffer_t *buf) 
{
  int lock_status;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufSize (buf %p)) "
                       "(Buffer is not locked.))",
                       buf, buf);
  }

  return buf->bufferCapacity - buf->bufferSpaceLeft;
}

snet_buffer_t *SNetBufCreate( unsigned int size, pthread_mutex_t *lock) {
  snet_buffer_t *theBuffer;
       
  theBuffer = SNetMemAlloc( sizeof( snet_buffer_t));

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

  theBuffer->flag = false;
  return( theBuffer);
}

snet_buffer_t *SNetBufPut( snet_buffer_t *bf, void* elem) {
  int lock_status;

  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufPut (buf %p)) (elem %p) "
                       "(Buffer is not locked))",
                       bf, bf, elem);
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
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufGet (buf %p)) "
                       "(Buffer is not locked))",
		       bf, bf);
  }

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

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufShow (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  if(SNetBufIsEmpty(buf)) {
    result = NULL;
  } else {
    result = buf->ringBuffer[buf->bufferEnd];
  }

  return result;
}
  
/* TODO: Rename to something like 'increase semaphore by record_count' */
extern int SNetBufRegisterDispatcher( snet_buffer_t *bf, snet_ex_sem_t *sem) {
  int i;
  int lock_status;
  int buffer_capacity;
  int space_left;
  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (BufRegisterDispatcher (bf %p) (sem %p))"
                       "(Buffer not locked))");
  }

  /* TODO: make this use getvalue of the semaphore? */
  buffer_capacity = bf->bufferCapacity;
  space_left = bf->bufferSpaceLeft;
  for( i=0; i<( buffer_capacity - space_left); i++) {
    SNetExSemIncrement( sem);
  }

  return buffer_capacity - space_left;
}

extern int SNetBufUnregisterDispatcher( snet_buffer_t *bf, snet_ex_sem_t *sem) {
  int i;
  int lock_status;
  int buffer_capacity;
  int space_left;
  lock_status = pthread_mutex_trylock(bf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (BufRegisterDispatcher (bf %p) (sem %p))"
                       "(Buffer not locked))");
  }


  /* TODO: make this use getvalue of the semaphore? */
  buffer_capacity = bf->bufferCapacity;
  space_left = bf->bufferSpaceLeft;

  for( i=0; i<( buffer_capacity - space_left); i++) {
    SNetExSemDecrement( sem);
  }

  return buffer_capacity - space_left;
}


extern void SNetBufDestroy( snet_buffer_t *bf) {
  SNetMemFree( bf->ringBuffer);
  SNetExSemDestroy(bf->record_count);
  SNetMemFree( bf);
}

extern bool SNetBufGetFlag(snet_buffer_t *buf)
{
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufGetFlag (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  return buf->flag;
}
  
extern void SNetBufSetFlag(snet_buffer_t *buf, bool flag)
{
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetBufSetFlag (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  buf->flag = flag;
}
