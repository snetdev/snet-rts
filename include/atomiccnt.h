/*****************************************************************************
 * Atomic counters
 *
 * Allow for atomic memory access and
 * fetch-and-increment and fetch-and-decrement
 * operations.
 *
 * Following datatypes and operations are supported:
 *
 * snet_atomiccnt_t               datatype for atomic counter
 * SNET_ATOMICCNT_INITIALIZER(i)  statically initialize with i
 *
 * Set an atomic counter to a specified value
 *    void SNetAtomicCntSet(snet_atomiccnt_t *cnt, int val);
 *
 * Atomic fetch-and-increment
 *    int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt);
 *
 * Atomic fetch-and-decrement
 *    int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt);
 *
 *
 * Auth: Daniel Prokesch <dlp@snet-home.org>
 * Date: 2011/08/02
 *
 ****************************************************************************/
#ifndef _ATOMICCNT_H_
#define _ATOMICCNT_H_

#ifndef LINE_SIZE
#define LINE_SIZE       64
#endif

#ifndef CAS
#define CAS(ptr, oldval, newval)   \
        __sync_bool_compare_and_swap(ptr, oldval, newval)
#endif

#ifdef HAVE_SYNC_ATOMIC_BUILTINS

typedef struct {
  volatile unsigned int counter;
  unsigned char padding[ LINE_SIZE - sizeof(int)];
} snet_atomiccnt_t;

#define SNET_ATOMICCNT_INITIALIZER(i)  {(i), {0}}

static inline void SNetAtomicCntSet(snet_atomiccnt_t *cnt, int val)
{ (void) __sync_lock_test_and_set(&cnt->counter, val); }

static inline unsigned int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt)
{ return __sync_fetch_and_add(&cnt->counter, 1); }

static inline unsigned int SNetAtomicCntIncAndFetch(snet_atomiccnt_t *cnt)
{ return __sync_add_and_fetch(&cnt->counter, 1); }

static inline unsigned int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt)
{ return __sync_fetch_and_sub(&cnt->counter, 1); }

static inline unsigned int SNetAtomicCntDecAndFetch(snet_atomiccnt_t *cnt)
{ return __sync_sub_and_fetch(&cnt->counter, 1); }

static inline unsigned int SNetAtomicCntGet(snet_atomiccnt_t *cnt)
{ return __sync_fetch_and_or(&cnt->counter, 0); }


#else /* HAVE_SYNC_ATOMIC_BUILTINS */


#include <pthread.h>

typedef struct {
  unsigned int counter;
  pthread_mutex_t lock;
  unsigned char padding[ LINE_SIZE - sizeof(int) - sizeof(pthread_mutex_t)];
} snet_atomiccnt_t;

#define SNET_ATOMICCNT_INITIALIZER(i)  {(i),PTHREAD_MUTEX_INITIALIZER}



static inline void SNetAtomicCntSet(snet_atomiccnt_t *cnt, int val)
{
  (void) pthread_mutex_lock( &cnt->lock);
  cnt->counter = val;
  (void) pthread_mutex_unlock( &cnt->lock);
}

static inline unsigned int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt)
{
  int tmp;
  (void) pthread_mutex_lock( &cnt->lock);
  tmp = cnt->counter;
  cnt->counter += 1;
  (void) pthread_mutex_unlock( &cnt->lock);
  return tmp;
}

static inline unsigned int SNetAtomicCntIncAndFetch(snet_atomiccnt_t *cnt)
{
  int tmp;
  (void) pthread_mutex_lock( &cnt->lock);
  cnt->counter += 1;
  tmp = cnt->counter;
  (void) pthread_mutex_unlock( &cnt->lock);
  return tmp;
}

static inline unsigned int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt)
{
  int tmp;
  (void) pthread_mutex_lock( &cnt->lock);
  tmp = cnt->counter;
  cnt->counter -= 1;
  (void) pthread_mutex_unlock( &cnt->lock);
  return tmp;
}

static inline unsigned int SNetAtomicCntDecAndFetch(snet_atomiccnt_t *cnt)
{
  int tmp;
  (void) pthread_mutex_lock( &cnt->lock);
  cnt->counter -= 1;
  tmp = cnt->counter;
  (void) pthread_mutex_unlock( &cnt->lock);
  return tmp;
}

static inline unsigned int SNetAtomicCntGet(snet_atomiccnt_t *cnt)
{
  int tmp;
  (void) pthread_mutex_lock( &cnt->lock);
  tmp = cnt->counter;
  (void) pthread_mutex_unlock( &cnt->lock);
  return tmp;
}

#endif /* HAVE_SYNC_ATOMIC_BUILTINS */

#endif /* _ATOMICCNT_H_ */
