#include "condif.h"
#include "bool.h"


void *condif( void *hnd, c4snet_data_t *p)
{
  bool bool_p = *(bool *) C4SNetGetData(p);

  if (bool_p) C4SNetOut(hnd, 1, 0);
  else C4SNetOut(hnd, 2, 0);

  C4SNetFree(p);

  return hnd;
}
