#include "bool.h"
#include "leq1.h"

void *leq1( void *hnd, c4snet_data_t *x)
{
  int int_x = *(int *) C4SNetGetData(x);
  bool bool_p = int_x <= 1;

  C4SNetOut(hnd, 1, x, C4SNetCreate(CTYPE_int, 1, &bool_p));

  return hnd;
}
