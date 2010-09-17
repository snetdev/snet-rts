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

static inline int fetch_and_inc( atomic_t *v )
{
  return __sync_fetch_and_add(&v->counter, 1);
}

/*
static inline int fetch_and_dec( atomic_t *v )
{
  return __sync_fetch_and_sub(&v->counter, 1);
}
*/
