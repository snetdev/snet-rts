#ifndef DISTRIBMAP_H
#define DISTRIBMAP_H

#include "bool.h"
#include "iomanagers.h"

#define MAP_NAME_H DestStream
#define MAP_TYPE_NAME_H dest_stream
#define MAP_KEY_H snet_dest_t*
#define MAP_VAL_H snet_stream_desc_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#define MAP_NAME_H StreamDest
#define MAP_TYPE_NAME_H stream_dest
#define MAP_KEY_H snet_stream_desc_t*
#define MAP_VAL_H snet_dest_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

typedef struct {
  snet_dest_t *dest;
  snet_stream_t *stream;
} snet_tuple_t;

#define LIST_NAME_H Tuple
#define LIST_TYPE_NAME_H tuple
#define LIST_VAL_H snet_tuple_t
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#endif
