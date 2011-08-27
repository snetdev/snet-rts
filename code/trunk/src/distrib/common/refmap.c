#include "refmap.h"

bool SNetRefCompare(snet_ref_t r1, snet_ref_t r2)
{
  return r1.node == r2.node && r1.data == r2.data;
}

#define MAP_NAME RefData
#define MAP_TYPE_NAME ref_data
#define MAP_KEY snet_ref_t
#define MAP_VAL data_t*
#define MAP_KEY_CMP SNetRefCompare
#include "map-template.c"
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME
