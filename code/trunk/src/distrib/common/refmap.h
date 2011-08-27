#ifndef REFMAP_H
#define REFMAP_H

#include "bool.h"
#include "distribution.h"

typedef struct {
  int refs;
  void *data;
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
