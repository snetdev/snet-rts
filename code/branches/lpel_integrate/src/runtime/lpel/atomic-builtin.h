/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

static inline void atomic_inc( atomic_t *v )
{
  (void)__sync_fetch_and_add(&v->counter, 1);
}

static inline int atomic_dec( atomic_t *v )
{
  return ( __sync_sub_and_fetch(&v->counter, 1)==0 ) ? 0 : 1;
}

static inline int atomic_swap( atomic_t *v, int value )
{
  return __sync_lock_test_and_set(&v->counter, value);
}

static inline int fetch_and_inc( atomic_t *v )
{
  return __sync_fetch_and_add(&v->counter, 1);
}

static inline int fetch_and_dec( atomic_t *v )
{
  return __sync_fetch_and_sub(&v->counter, 1);
}
