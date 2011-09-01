#include "iomanagers.h"
#include "distribcollections.h"

#define MAP_NAME DestStream
#define MAP_TYPE_NAME dest_stream
#define MAP_KEY snet_dest_t*
#define MAP_VAL snet_stream_desc_t*
#define MAP_KEY_CMP SNetDestCompare
#define MAP_KEY_FREE_FUN SNetDestFree
#include "map-template.c"
#undef MAP_KEY_FREE_FUN
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

#define MAP_NAME StreamDest
#define MAP_TYPE_NAME stream_dest
#define MAP_KEY snet_stream_desc_t*
#define MAP_VAL snet_dest_t*
#include "map-template.c"
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

bool SNetTupleCompare(snet_tuple_t t1, snet_tuple_t t2)
{
  (void) t1; /* NOT USED */
  (void) t2; /* NOT USED */
  return false;
}

#define LIST_NAME Tuple
#define LIST_TYPE_NAME tuple
#define LIST_VAL snet_tuple_t
#define LIST_CMP SNetTupleCompare
#include "list-template.c"
#undef LIST_CMP
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
