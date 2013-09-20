#ifndef RESDEFS_H_INCLUDED
#define RESDEFS_H_INCLUDED

#include <stdarg.h>
#ifndef NULL
#include <stdlib.h>
#endif
#ifndef __bool_true_false_are_defined
#include <stdbool.h>
#endif
#ifndef ULONG_MAX
#include <limits.h>
#endif

#define RES_DEFAULT_LISTEN_PORT 56389

#define LOCAL_HOST      0
#define DEPTH_ZERO      0
#define NO_PARENT       NULL
#define MAX_LOGICAL     MAX_BIT

#define BITMAP_ZERO     0UL
#define BITMAP_ALL      ULONG_MAX

#define NUM_BITS        ((int) (8 * sizeof(bitmap_t)))
#define MAX_BIT         ((int) (8 * sizeof(bitmap_t) - 1))

#define HAS(map, bit)   ((map)  &  (1UL << (bit)))
#define SET(map, bit)   ((map) |=  (1UL << (bit)))
#define CLR(map, bit)   ((map) &= ~(1UL << (bit)))
#define NOT(map, bit)   (HAS((map), (bit)) == 0)
#define ZERO(map)       ((map) = BITMAP_ZERO)

#define xnew(x)         ((x *) xmalloc(sizeof(x)))

#if NDEBUG
#define res_assert(b,m) ((void)0)
#else
#define res_assert(b,m) ((b)?((void)0):res_assert_failed(__FILE__,__LINE__,(m)))
#endif

typedef unsigned long bitmap_t;

#include "restypes.h"
#include "resproto.h"

#endif
