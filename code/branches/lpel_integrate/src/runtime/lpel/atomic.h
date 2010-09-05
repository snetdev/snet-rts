#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/**
 * Atomic variables and
 * TODO swap and membars
 *
 */

#define asm __asm__

typedef struct {
  volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v) ((v)->counter)


/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
#define atomic_set(v,i) (((v)->counter) = (i))



/*NOTE: XADD only available since 486 */
#if defined(__x86_64__) || defined(__i386__)

/**
 * Increment atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_inc( atomic_t *v )
{
  volatile int *cnt = &v->counter;
  asm volatile ("lock; incl %0"
      : /* no output */
      : "m" (*cnt)
      : "memory", "cc");
}

/**
 * Decrement atomic variable
 * @param v: pointer of type atomic_t
 * @return 0 if the variable is 0 _after_ the decrement.
 *
 * Atomically decrements @v by 1
 */
static inline int atomic_dec( atomic_t *v )
{
  volatile int *cnt = &v->counter;
  unsigned char prev = 0;
  asm volatile ("lock; decl %0; setnz %1"
      : "=m" (*cnt), "=qm" (prev)
      : "m" (*cnt)
      : "memory", "cc");
  return (int)prev;
}


/**
 * Atomic fetch and increment
 * @param v: pointer of type atomic_t
 * @return the value before incrementing
 */
static inline int fetch_and_inc( atomic_t *v )
{
  volatile int *cnt = &v->counter;
  int tmp = 1;
  asm volatile("lock; xadd%z0 %0,%1"
      : "=r" (tmp), "=m" (*cnt)
      : "0" (tmp), "m" (*cnt)
      : "memory", "cc");
  return tmp;
}


/**
 * Atomic fetch and decrement
 * @param v: pointer of type atomic_t
 * @return the value before decrementing
 */
static inline int fetch_and_dec( atomic_t *v )
{
  volatile int *cnt = &v->counter;
  int tmp = -1;
  asm volatile("lock; xadd%z0 %0,%1"
      : "=r" (tmp), "=m" (*cnt)
      : "0" (tmp), "m" (*cnt)
      : "memory", "cc");
  return tmp;
}
#else
#warning "Fallback to compiler builtin atomics (icc/gcc)!"

static inline void atomic_inc( atomic_t *v )
{
  (void)__sync_fetch_and_add(&v->counter, 1);
}

static inline int atomic_dec( atomic_t *v )
{
  return ( __sync_sub_and_fetch(&v->counter, 1)==0 ) ? 0 : 1;
}

static inline int fetch_and_inc( atomic_t *v )
{
  return __sync_fetch_and_add(&v->counter, 1);
}

static inline int fetch_and_dec( atomic_t *v )
{
  return __sync_fetch_and_sub(&v->counter, 1);
}

#endif /* defined(__x86_64) || defined(__i386__) */

#endif /* _ATOMIC_H_ */
