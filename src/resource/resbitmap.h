#ifndef RESBITMAP_H_INCLUDED
#define RESBITMAP_H_INCLUDED

#ifndef ULONG_MAX
#include <limits.h>
#endif

#define BITMAP_ZERO     0UL
#define BITMAP_ALL      ULONG_MAX

#define NUM_BITS        ((int) (8 * sizeof(bitmap_t)))
#define MAX_BIT         (NUM_BITS - 1)

#define HAS(map, bit)   ((map)  &  (1UL << (bit)))
#define SET(map, bit)   ((map) |=  (1UL << (bit)))
#define CLR(map, bit)   ((map) &= ~(1UL << (bit)))
#define NOT(map, bit)   (HAS((map), (bit)) == 0)
#define ZERO(map)       ((map) = BITMAP_ZERO)
#define NON_ZERO(map)   ((map) != 0UL)

#define BITMAP_EQ(a,b)  ((a) == (b))
#define BITMAP_NEQ(a,b) ((a) != (b))
#define BITMAP_OR(a,b)  ((a) | (b))
#define BITMAP_AND(a,b) ((a) & (b))
#define BITMAP_NOT(a)   (~(a))

typedef unsigned long   bitmap_t;

#endif
