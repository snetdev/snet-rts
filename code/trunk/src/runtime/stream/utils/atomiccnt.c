

#include "atomiccnt.h"

#include "memfun.h"

#ifdef HAVE_SYNC_ATOMIC_BUILTINS

struct snet_atomiccnt {
  volatile int counter;
  unsigned char padding[64-sizeof(int)];
};


snet_atomiccnt_t *SNetAtomicCntCreate(int val)
{
  snet_atomiccnt_t *cnt = SNetMemAlloc(sizeof(snet_atomiccnt_t));
  cnt->counter = val;
  return cnt;
}

void SNetAtomicCntDestroy(snet_atomiccnt_t *cnt)
{ SNetMemFree(cnt); }

int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt)
{ return __sync_fetch_and_add(&cnt->counter, 1); }


int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt)
{ return __sync_fetch_and_sub(&cnt->counter, 1); }


#else /* HAVE_SYNC_ATOMIC_BUILTINS */


#include <pthread.h>

struct snet_atomiccnt {
  int counter;
  pthread_mutex_t lock;
};


snet_atomiccnt_t *SNetAtomicCntCreate(int val)
{
  snet_atomiccnt_t *cnt = SNetMemAlloc(sizeof(snet_atomiccnt_t));
  (void) pthread_mutex_init( &cnt->lock, NULL);
  cnt->counter = val;
  return cnt;
}

void SNetAtomicCntDestroy(snet_atomiccnt_t *cnt)
{
  (void) pthread_mutex_destroy( &cnt->lock);
  SNetMemFree(cnt);
}

int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt)
{
  int tmp;

  (void) pthread_mutex_lock( &cnt->lock);
  tmp = cnt->counter;
  cnt->counter += 1;
  (void) pthread_mutex_unlock( &cnt->lock);

  return tmp;
}

int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt)
{
  int tmp;

  (void) pthread_mutex_lock( &cnt->lock);
  tmp = cnt->counter;
  cnt->counter -= 1;
  (void) pthread_mutex_unlock( &cnt->lock);

  return tmp;
}



#endif /* HAVE_SYNC_ATOMIC_BUILTINS */
