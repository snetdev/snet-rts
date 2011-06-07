#include "map.h"

/* !! CAUTION !!
 * These includes are mirrored in include/map.h, keep in sync under penalty of
 * defenestration!
 */
#define MAP_NAME_C Int
#define MAP_TYPE_NAME_C int
#define MAP_VAL_C int
#include "map-template.c"
#undef MAP_VAL_C
#undef MAP_TYPE_NAME_C
#undef MAP_NAME_C

#define MAP_NAME_C Void
#define MAP_TYPE_NAME_C void
#define MAP_VAL_C void*
#include "map-template.c"
#undef MAP_VAL_C
#undef MAP_TYPE_NAME_C
#undef MAP_NAME_C
