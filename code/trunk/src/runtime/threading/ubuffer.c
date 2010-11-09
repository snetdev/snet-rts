#include <pthread.h>
#include <errno.h>
#include <pthread.h>

#include "ubuffer.h"
#include "constants.h"
#include "bool.h"
#include "memfun.h"
#include "extended_semaphore.h"
#include "stream_layer.h"
#include "debug.h"

struct snet_ubuffer  {
  bool flag;
  void **buf;
  unsigned int head;
  unsigned int tail;
  unsigned int elements;
  unsigned int size;

  pthread_mutex_t *mxCounter;
  snet_ex_sem_t *record_count;
};

extern bool SNetUBufIsEmpty(snet_ubuffer_t *buf) 
{
  int lock_status;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufIsEmpty (buf %p)) "
                       "(Buffer is not locked.))",
                       buf, buf);
  }

  return (buf->elements == 0);
}

extern unsigned int SNetUBufSize(snet_ubuffer_t *buf)
{
  int lock_status;
  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufIsEmpty (buf %p)) "
                       "(Buffer is not locked.))",
                       buf, buf);
  }

  return buf->elements;
}

extern snet_ubuffer_t *SNetUBufCreate(pthread_mutex_t *lock) 
{
  snet_ubuffer_t *new;
       
  new = SNetMemAlloc( sizeof( snet_ubuffer_t));

  new->size = BUFFER_SIZE;
  new->buf = SNetMemAlloc(new->size * sizeof( void**));
  new->tail = 0;
  new->head = 0;
  new->elements = 0;
  new->flag = false;

  new->mxCounter = lock;
  new->record_count = SNetExSemCreate(lock, true, 0, false, 0, 0);

  return new;
}

extern bool SNetUBufPut( snet_ubuffer_t *buf, void* elem) 
{
  int lock_status;
  void **temp;
  int i;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufPut (buf %p)) (elem %p) "
                       "(Buffer is not locked))",
                       buf, buf, elem);
  }

  buf->record_count = SNetExSemIncrement(buf->record_count);

  if(buf->elements == buf->size) {
    if((temp = SNetMemAlloc((buf->size + BUFFER_SIZE) * sizeof(void **))) == NULL) {
      return false;
    }
    
    i = 0; 
    do {
      temp[i] = buf->buf[buf->head];
      buf->head = (buf->head + 1) % buf->size;
    } while(buf->head != buf->tail);

    buf->head = 0;
    buf->tail = buf->elements;
    buf->size += BUFFER_SIZE;

    SNetMemFree(buf->buf);

    buf->buf = temp;
  }

  buf->buf[buf->tail] = elem;
  buf->tail = (buf->tail + 1) % buf->size;
  buf->elements++;
	
  return true;
}

extern void *SNetUBufGet( snet_ubuffer_t *buf) 
{
  void *result;
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);
  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufGet (buf %p)) "
                       "(Buffer is not locked))",
		       buf, buf);
  }

  buf->record_count = SNetExSemDecrement(buf->record_count);

  /* get the data from the buffer*/
  result = (buf->buf[buf->head]);   
  buf->head = (buf->head + 1) % buf->size;
  buf->elements--;
  
  return result;
}

extern void *SNetUBufShow( snet_ubuffer_t *buf) 
{
  int lock_status;

  void *result = NULL;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufShow (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  if(buf->elements > 0) {
    result = buf->buf[buf->head];
  }
    
  return result;
}

extern bool SNetUBufGetFlag(snet_ubuffer_t *buf)
{
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufShow (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  return buf->flag;
}
  
extern void SNetUBufSetFlag(snet_ubuffer_t *buf, bool flag)
{
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (SNetUBufShow (buf %p)) "
                       "(Buffer is not locked))", buf, 
                       buf);
  }

  buf->flag = flag;
}

extern int SNetUBufRegisterDispatcher( snet_ubuffer_t *buf, snet_ex_sem_t *sem) 
{
  int i;
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (BufRegisterDispatcher (bf %p) (sem %p))"
                       "(Buffer not locked))");
  }

  for(i = 0; i< buf->elements; i++) {
    SNetExSemIncrement( sem);
  }

  return buf->elements;
}

extern int SNetUBufUnregisterDispatcher( snet_ubuffer_t *buf, snet_ex_sem_t *sem) 
{
  int i;
  int lock_status;

  lock_status = pthread_mutex_trylock(buf->mxCounter);

  if(lock_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (BUFFER %p) (BufRegisterDispatcher (bf %p) (sem %p))"
                       "(Buffer not locked))");
  }

  for(i = 0; i< buf->elements; i++) {
    SNetExSemDecrement( sem);
  }

  return buf->elements;
}

extern void SNetUBufDestroy( snet_ubuffer_t *buf) 
{
  SNetMemFree(buf->buf);
  SNetExSemDestroy(buf->record_count);
  SNetMemFree(buf);
}
