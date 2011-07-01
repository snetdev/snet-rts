#include "distribmap.h"

extern bool SNetDestCompare(snet_dest_t, snet_dest_t);

#define MAP_NAME DestStream
#define MAP_TYPE_NAME dest_stream
#define MAP_KEY snet_dest_t
#define MAP_VAL snet_stream_desc_t*
#define MAP_KEY_CMP SNetDestCompare
#include "map-template.c"
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

#define MAP_NAME StreamDest
#define MAP_TYPE_NAME stream_dest
#define MAP_KEY snet_stream_desc_t*
#define MAP_VAL snet_dest_t
#include "map-template.c"
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME
