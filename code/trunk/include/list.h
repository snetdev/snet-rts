#ifndef LIST_H
#define LIST_H

#include "bool.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/list.c, keep in sync
 * under penalty of defenestration!
 */
#define LIST_NAME Int
#define LIST_TYPE_NAME int
#define LIST_VAL int
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#include "variant.h"

#define LIST_NAME Variant
#define LIST_TYPE_NAME variant
#define LIST_VAL snet_variant_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#define LIST_NAME VariantList
#define LIST_TYPE_NAME variant_list
#define LIST_VAL snet_variant_list_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#include "expression.h"

#define LIST_NAME Expr
#define LIST_TYPE_NAME expr
#define LIST_VAL snet_expr_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#include "filter.h"

#define LIST_NAME FilterInstr
#define LIST_TYPE_NAME filter_instr
#define LIST_VAL snet_filter_instr_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#define LIST_NAME FilterInstrList
#define LIST_TYPE_NAME filter_instr_list
#define LIST_VAL snet_filter_instr_list_t*
#include "list-template.h"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#endif
