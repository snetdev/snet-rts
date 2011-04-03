#ifndef MAP_H
#define MAP_H

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/map.c, keep in sync under
 * penalty of defenestration!
 */
#define MAP_NAME_H int
#define MAP_VAL_H int
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_NAME_H

#include "distribution.h"

#define MAP_NAME_H ref
#define MAP_VAL_H snet_ref_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_NAME_H

#endif
