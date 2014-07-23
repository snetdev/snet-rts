#include <stdlib.h>
#include <unistd.h>
#include <bool.h>
#include <stdio.h>

#include "ident.h"

void *ident( void *hnd, c4snet_data_t *x, int T)
{
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetGetData( x);
  //int_x -= 1;

  result = C4SNetCreate(CTYPE_int, 1, &int_x);

  C4SNetFree(x);

  usleep(1230);

  C4SNetOut( hnd, 1, result, T+1);
  return( hnd);
}

