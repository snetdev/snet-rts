#ifndef REFMAP_H
#define REFMAP_H

#include "bool.h"
#include "distribution.h"

#define LIST_NAME_H Stream
#define LIST_TYPE_NAME_H stream
#define LIST_VAL_H snet_stream_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

typedef struct {
  int refs;
  void *data;
  snet_stream_list_t *list;
} data_t;

#define MAP_NAME_H RefData
#define MAP_TYPE_NAME_H ref_data
#define MAP_KEY_H snet_ref_t
#define MAP_VAL_H data_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#endif
