#include "bool.h"
#include "mult.h"

void *mult( void *hnd, c4snet_data_t *x, c4snet_data_t *r)
{
  int int_x = *(int *) C4SNetGetData(x),
      int_r = *(int *) C4SNetGetData(r),
      int_rr = int_x * int_r;

  C4SNetFree(x);
  C4SNetFree(r);

  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_rr));
  return hnd;
}
