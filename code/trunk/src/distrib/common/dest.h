#ifndef DEST_H
#define DEST_H

#include "bool.h"

typedef struct snet_dest {
  int node,
      dest,
      parent,
      parentNode,
      dynamicIndex,
      dynamicLoc;
} snet_dest_t;

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2);
snet_dest_t *SNetDestCopy(snet_dest_t *dest);
void SNetDestFree(snet_dest_t *dest);

#endif
