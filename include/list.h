#ifndef LIST_H
#define LIST_H

#include "bool.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime/common/list.c, keep in sync
 * under penalty of defenestration!
 */
#define LIST_NAME_H Int
#define LIST_TYPE_NAME_H int
#define LIST_VAL_H int
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define LIST_NAME_H IntList
#define LIST_TYPE_NAME_H int_list
#define LIST_VAL_H snet_int_list_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#include "variant.h"

#define LIST_NAME_H Variant
#define LIST_TYPE_NAME_H variant
#define LIST_VAL_H snet_variant_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define LIST_NAME_H VariantList
#define LIST_TYPE_NAME_H variant_list
#define LIST_VAL_H snet_variant_list_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#include "expression.h"

#define LIST_NAME_H Expr
#define LIST_TYPE_NAME_H expr
#define LIST_VAL_H snet_expr_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#include "filter.h"

#define LIST_NAME_H FilterInstr
#define LIST_TYPE_NAME_H filter_instr
#define LIST_VAL_H snet_filter_instr_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define LIST_NAME_H FilterInstrList
#define LIST_TYPE_NAME_H filter_instr_list
#define LIST_VAL_H snet_filter_instr_list_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#endif
