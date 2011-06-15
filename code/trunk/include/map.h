#ifndef MAP_H
#define MAP_H

#include "bool.h"

/* !! CAUTION !!
 * These includes are mirrored in runtime_lpel/shared/map.c, keep in sync under
 * penalty of defenestration!
 */
#define MAP_NAME_H Int
#define MAP_TYPE_NAME_H int
#define MAP_VAL_H int
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#define MAP_NAME_H Void
#define MAP_TYPE_NAME_H void
#define MAP_VAL_H void*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#endif
