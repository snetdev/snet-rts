#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "list.h"

/* !! CAUTION !!
 * These includes are mirrored in include/list.h, keep in sync under penalty of
 * defenestration!
 */
#define LIST_NAME Int
#define LIST_TYPE_NAME int
#define LIST_VAL int
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#define LIST_NAME IntList
#define LIST_TYPE_NAME int_list
#define LIST_VAL snet_int_list_t*
#define LIST_FREE_FUNCTION SNetIntListDestroy
#include "list-template.c"
#undef LIST_FREE_FUNCTION
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME


#define LIST_NAME Variant
#define LIST_TYPE_NAME variant
#define LIST_VAL snet_variant_t*
#define LIST_FREE_FUNCTION SNetVariantDestroy
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
#undef LIST_FREE_FUNCTION

#define LIST_NAME VariantList
#define LIST_TYPE_NAME variant_list
#define LIST_VAL snet_variant_list_t*
#define LIST_FREE_FUNCTION SNetVariantListDestroy
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
#undef LIST_FREE_FUNCTION

#include "expression.h"

#define LIST_NAME Expr
#define LIST_TYPE_NAME expr
#define LIST_VAL snet_expr_t*
#define LIST_FREE_FUNCTION SNetExprDestroy
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
#undef LIST_FREE_FUNCTION

#define LIST_NAME FilterInstr
#define LIST_TYPE_NAME filter_instr
#define LIST_VAL snet_filter_instr_t*
#define LIST_FREE_FUNCTION SNetDestroyFilterInstruction
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
#undef LIST_FREE_FUNCTION

#define LIST_NAME FilterInstrList
#define LIST_TYPE_NAME filter_instr_list
#define LIST_VAL snet_filter_instr_list_t*
#define LIST_FREE_FUNCTION SNetFilterInstrListDestroy
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
#undef LIST_FREE_FUNCTION



