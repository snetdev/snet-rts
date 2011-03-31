#ifndef LIST_H
#define LIST_H

#include "filter.h"
#include "expression.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/list.c, keep in sync
 * under penalty of defenestration!
 */
#define LIST_NAME filter_instruction
#define LIST_VAL snet_filter_instruction_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_NAME

#define LIST_NAME filter_instruction_list
#define LIST_VAL snet_filter_instruction_list_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_NAME

#define LIST_NAME expr
#define LIST_VAL snet_expr_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_NAME

#endif
