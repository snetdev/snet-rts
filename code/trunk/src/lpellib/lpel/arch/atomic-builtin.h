/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

/**
 * Atomic variables and
 * TODO pointer swap and membars
 *
 */


typedef struct {
  volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

/**
 * Initialize atomic variable dynamically
 */
#define atomic_init(v,i)  atomic_set((v),(i))

/**
 * Destroy atomic variable
 */
#define atomic_destroy(v)  /*NOP*/

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

static inline int compare_and_swap( void**ptr, void* oldval, void* newval)
{
  return __sync_bool_compare_and_swap (ptr, oldval, newval);
}

