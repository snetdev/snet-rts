#ifndef DISTRIBMAP_H
#define DISTRIBMAP_H

#include "bool.h"
#include "threading.h"

typedef struct {
  int node,
      dest,
      dynamicParent;
} snet_dest_t;

#define MAP_NAME_H DestStream
#define MAP_TYPE_NAME_H dest_stream
#define MAP_KEY_H snet_dest_t
#define MAP_VAL_H snet_stream_desc_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#define MAP_NAME_H StreamDest
#define MAP_TYPE_NAME_H stream_dest
#define MAP_KEY_H snet_stream_desc_t*
#define MAP_VAL_H snet_dest_t
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#endif
