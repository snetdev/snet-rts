#ifndef MAP_H
#define MAP_H

#include "bool.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/map.c, keep in sync under
 * penalty of defenestration!
 */
#define MAP_NAME Int
#define MAP_TYPE_NAME int
#define MAP_VAL int
#include "map-template.h"
#undef MAP_VAL
#undef MAP_TYPE_NAME
#undef MAP_NAME

#include "distribution.h"

#define MAP_NAME Ref
#define MAP_TYPE_NAME ref
#define MAP_VAL snet_ref_t*
#include "map-template.h"
#undef MAP_VAL
#undef MAP_TYPE_NAME
#undef MAP_NAME

#endif
