#include <stdlib.h>
#include <stdio.h>

#include "delay.h"

void *delay( void *hnd, c4snet_data_t *x, int T)
{
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetGetData( x);
  int_x *= 100;

  /* delay */
  usleep(int_x);

  result = C4SNetCreate(CTYPE_int, 1, &int_x);

  C4SNetFree(x);

  C4SNetOut( hnd, 1, result, T);
  return( hnd);
}

