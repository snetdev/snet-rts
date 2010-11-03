#ifndef _ATOMIC_H_
#define _ATOMIC_H_


#if  (__GNUC__ > 4) || \
  (__GNUC__==4) && (__GNUC_MINOR__ >= 1) && (__GNUC_PATCHLEVEL__ >= 0)
/* gcc/icc compiler builtin atomics  */
#include "atomic-builtin.h"


#elif defined(__x86_64__) || defined(__i386__)
/*NOTE: XADD only available since 486, ignore this fact for now,
        as 386's are uncommon nowadays */
#include "atomic-x86.h"


#else
#warning "Cannot determine which atomics to use, fallback to pthread locked atomics!"
#include "atomic-pthread.h"

#endif


#endif /* _ATOMIC_H_ */
