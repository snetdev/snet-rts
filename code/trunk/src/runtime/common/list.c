#include "list.h"

/* !! CAUTION !!
 * These includes are mirrored in include/list.h, keep in sync under penalty of
 * defenestration!
 */

#define LIST_NAME_C filter_instr
#define LIST_VAL_C snet_filter_instr_t*
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_NAME_C

#define LIST_NAME_C filter_instr_list
#define LIST_VAL_C snet_filter_instr_list_t*
#include "list-template.c"
#undef LIST_VAL_C
#undef LIST_NAME_C
