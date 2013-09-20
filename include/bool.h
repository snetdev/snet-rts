/*
 * Datatype Boolean
 */

#ifndef _SNETC_TYPES_H_ /* in case this gets included when building snetc */
#ifndef __bool_true_false_are_defined   /* be exclusive wrt. stdbool.h. */
#define __bool_true_false_are_defined   1

typedef int bool;
#define false 0
#define true  1

#endif
#endif /* _SNETC_TYPES_H_  */
