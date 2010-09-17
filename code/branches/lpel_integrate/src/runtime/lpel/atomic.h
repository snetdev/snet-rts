#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/**
 * Atomic variables and
 * TODO swap and membars
 *
 */


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

#if  (__GNUC__ > 4) || \
  (__GNUC__==4) && (__GNUC_MINOR__ >= 1) && (__GNUC_PATCHLEVEL__ >= 0)
/* gcc/icc compiler builtin atomics  */
#include "atomic-builtin.h"


#elif defined(__x86_64__) || defined(__i386__)
/*NOTE: XADD only available since 486, ignore this fact for now,
        as 386's are uncommon nowadays */
#include "atomic-x86.h"


#else
#error "Cannot determine which atomics to use!"

#endif

#endif /* _ATOMIC_H_ */
