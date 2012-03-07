#include "bool.h"
#include "sub.h"

void *sub( void *hnd, c4snet_data_t *x)
{
  int int_x = *(int *) C4SNetGetData(x);

  int_x -= 1;

  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_x));
  C4SNetFree(x);

  return hnd;
}
