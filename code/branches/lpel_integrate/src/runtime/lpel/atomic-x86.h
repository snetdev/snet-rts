/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

#define asm __asm__

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
 /*
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
*/

