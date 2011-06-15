#include "map.h"

/* !! CAUTION !!
 * These includes are mirrored in include/map.h, keep in sync under penalty of
 * defenestration!
 */
#define MAP_NAME Int
#define MAP_TYPE_NAME int
#define MAP_VAL int
#include "map-template.c"
#undef MAP_VAL
#undef MAP_TYPE_NAME
#undef MAP_NAME

#define MAP_NAME Void
#define MAP_TYPE_NAME void
#define MAP_VAL void*
#include "map-template.c"
#undef MAP_VAL
#undef MAP_TYPE_NAME
#undef MAP_NAME
