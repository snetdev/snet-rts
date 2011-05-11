#include "list.h"

/* !! CAUTION !!
 * These includes are mirrored in include/list.h, keep in sync under penalty of
 * defenestration!
 */
#define LIST_NAME_C Int
#define LIST_TYPE_NAME_C int
#define LIST_VAL_C int
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C

#define LIST_NAME_C Variant
#define LIST_TYPE_NAME_C variant
#define LIST_VAL_C snet_variant_t*
#define LIST_FREE_FUNCTION SNetVariantDestroy
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C
#undef LIST_FREE_FUNCTION

#define LIST_NAME_C VariantList
#define LIST_TYPE_NAME_C variant_list
#define LIST_VAL_C snet_variant_list_t*
#define LIST_FREE_FUNCTION SNetVariantListDestroy
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C
#undef LIST_FREE_FUNCTION

#include "expression.h"

#define LIST_NAME_C Expr
#define LIST_TYPE_NAME_C expr
#define LIST_VAL_C snet_expr_t*
#define LIST_FREE_FUNCTION SNetExprDestroy
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C
#undef LIST_FREE_FUNCTION

#define LIST_NAME_C FilterInstr
#define LIST_TYPE_NAME_C filter_instr
#define LIST_VAL_C snet_filter_instr_t*
#define LIST_FREE_FUNCTION SNetDestroyFilterInstruction
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C
#undef LIST_FREE_FUNCTION

#define LIST_NAME_C FilterInstrList
#define LIST_TYPE_NAME_C filter_instr_list
#define LIST_VAL_C snet_filter_instr_list_t*
#define LIST_FREE_FUNCTION SNetFilterInstrListDestroy
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_TYPE_NAME_C
#undef LIST_NAME_C
#undef LIST_FREE_FUNCTION
