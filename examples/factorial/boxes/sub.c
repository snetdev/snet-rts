#include "bool.h"
#include "sub.h"

void *sub( void *hnd, c4snet_data_t *x)
{
  c4snet_container_t *c;
  int int_x = *(int *) C4SNetGetData(x);

  int_x -= 1;

  c = C4SNetContainerCreate(hnd, 1);
  C4SNetContainerSetField(c, C4SNetCreate(CTYPE_int, 1, &int_x));
  C4SNetContainerOut(c);

  C4SNetFree(x);

  return hnd;
}
