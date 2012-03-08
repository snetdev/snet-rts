#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

#include "dec.h"

void *dec( void *hnd, c4snet_data_t *x)
{
  int int_x = *(int *) C4SNetGetData(x);
  int_x -= 1;

  C4SNetFree(x);
  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_x));

  return hnd;
}
