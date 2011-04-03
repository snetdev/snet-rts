#ifndef LIST_H
#define LIST_H

#include "filter.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/list.c, keep in sync
 * under penalty of defenestration!
 */
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
