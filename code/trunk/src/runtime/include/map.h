#ifndef MAP_H
#define MAP_H

#include "distribution.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/map.c, keep in sync under
 * penalty of defenestration!
 */
#define MAP_NAME int
#define MAP_VAL int
#include "map-template.h"
#undef MAP_VAL
#undef MAP_NAME

#define MAP_NAME ref
#define MAP_VAL snet_ref_t*
#include "map-template.h"
#undef MAP_VAL
#undef MAP_NAME

#endif
