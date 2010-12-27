/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

/**
 * Atomic variables and
 * TODO pointer swap and membars
 *
 */

#include <pthread.h>

#define ATOMIC_PTHREAD_USE_SPINLOCK

typedef struct {
  int counter;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  pthread_spinlock_t lock;
#else
  pthread_mutex_t lock;
#endif
} atomic_t;

#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
#  define ATOMIC_INIT(i) { (i), 1 }
#else
#  define ATOMIC_INIT(i) { (i), PTHREAD_MUTEX_INITIALIZER }
#endif

/**
 * Initialize atomic variable dynamically
 */
static inline void atomic_init( atomic_t *v, int i )
{
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_init( &v->lock, PTHREAD_PROCESS_PRIVATE);
#else
  (void) pthread_mutex_init( &v->lock, NULL);
#endif
  v->counter = i;
}

/**
 * Destroy atomic variable
 */
static inline void atomic_destroy( atomic_t *v )
{
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_destroy( &v->lock);
#else
  (void) pthread_mutex_destroy( &v->lock);
#endif
}


/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
static inline int atomic_read( atomic_t *v )
{
  int tmp;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  tmp = v->counter;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
  return tmp;
}


/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
static inline void atomic_set( atomic_t *v, int i )
{
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  v->counter = i;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
}

static inline void atomic_inc( atomic_t *v )
{
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  v->counter++;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
}

static inline int atomic_dec( atomic_t *v )
{
  int tmp;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  v->counter -= 1;
  tmp = v->counter;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
  return ( tmp == 0 ) ? 0 : 1;
}

static inline int atomic_swap( atomic_t *v, int value )
{
  int tmp;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  tmp = v->counter;
  v->counter = value;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
  return tmp;
}

static inline int fetch_and_inc( atomic_t *v )
{
  int tmp;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  tmp = v->counter;
  v->counter += 1;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
  return tmp;
}

static inline int fetch_and_dec( atomic_t *v )
{
  int tmp;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_lock( &v->lock);
#else
  (void) pthread_mutex_lock( &v->lock);
#endif
  tmp = v->counter;
  v->counter -= 1;
#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
  (void) pthread_spin_unlock( &v->lock);
#else
  (void) pthread_mutex_unlock( &v->lock);
#endif
  return tmp;
}

