#include "map.h"

/* !! CAUTION !!
 * These includes are mirrored in include/map.h, keep in sync under penalty of
 * defenestration!
 */
#define MAP_NAME_C int
#define MAP_VAL_C int
#include "map-template.c"
#undef MAP_VAL_C
#undef MAP_NAME_C

#define MAP_NAME_C ref
#define MAP_VAL_C snet_ref_t*
#include "map-template.c"
#undef MAP_VAL_C
#undef MAP_NAME_C
