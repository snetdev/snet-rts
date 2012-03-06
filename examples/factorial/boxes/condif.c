#include "condif.h"
#include "bool.h"


void *condif( void *hnd, c4snet_data_t *p)
{
  bool bool_p = *(bool *) C4SNetGetData(p);

  c4snet_container_t *c = C4SNetContainerCreate(hnd, bool_p ? 1 : 2);
  C4SNetContainerSetTag(c, 0);
  C4SNetContainerOut(c);

  C4SNetFree(p);

  return hnd;
}
