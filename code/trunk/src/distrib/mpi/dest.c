#include "bool.h"
#include "dest.h"
#include "iomanagers.h"
#include "memfun.h"

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2)
{
  return d1->node == d2->node && d1->dest == d2->dest &&
         d1->parent == d2->parent && d1->dynamicIndex == d2->dynamicIndex;
}

snet_dest_t *SNetDestCopy(snet_dest_t *d)
{
  snet_dest_t *result = SNetMemAlloc(sizeof(snet_dest_t));
  *result = *d;
  return result;
}

void SNetDestFree(snet_dest_t *d)
{
  SNetMemFree(d);
}
