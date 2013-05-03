#include <stdlib.h>
#include <bool.h>
#include <stdio.h>
#include <unistd.h>

#include <inc.h>

void *inc( void *hnd, c4snet_data_t *x)
{
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetGetData( x);

  usleep((int_x % 4) * 10000);

  int_x += 1;

  result = C4SNetCreate(CTYPE_int, 1, &int_x);

  C4SNetFree(x);

  C4SNetOut( hnd, 1, result);

  usleep(((int_x - 1) % 4) * 10000);

  return( hnd);
}

