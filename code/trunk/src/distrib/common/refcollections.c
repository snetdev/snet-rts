#include "refcollections.h"

#define LIST_NAME Stream
#define LIST_TYPE_NAME stream
#define LIST_VAL snet_stream_t*
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

bool SNetRefCompare(snet_ref_t r1, snet_ref_t r2)
{
  return r1.node == r2.node && r1.data == r2.data;
}

#define MAP_NAME RefRefcount
#define MAP_TYPE_NAME ref_refcount
#define MAP_KEY snet_ref_t
#define MAP_VAL snet_refcount_t*
#define MAP_KEY_CMP SNetRefCompare
#include "map-template.c"
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME
