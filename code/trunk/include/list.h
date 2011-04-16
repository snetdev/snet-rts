#ifndef LIST_H
#define LIST_H

#include "bool.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/list.c, keep in sync
 * under penalty of defenestration!
 */
#define LIST_NAME_H int
#define LIST_VAL_H int
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_NAME_H

#include "variant.h"

#define LIST_NAME_H variant
#define LIST_VAL_H snet_variant_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_NAME_H

#define LIST_NAME_H variant_list
#define LIST_VAL_H snet_variant_list_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_NAME_H

#include "filter.h"

#define LIST_NAME_H filter_instr
#define LIST_VAL_H snet_filter_instr_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_NAME_H

#define LIST_NAME_H filter_instr_list
#define LIST_VAL_H snet_filter_instr_list_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_NAME_H

#endif
