#include <stdlib.h>
#include <unistd.h>
#include <bool.h>
#include <stdio.h>

#include "ident.h"

void *ident( void *hnd, c4snet_data_t *x, int T)
{
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetDataGetData( x);
  //int_x -= 1;

  result = C4SNetDataCreate( CTYPE_int, &int_x);

  C4SNetDataFree(x);

  usleep(1000*(1000+200+30));

  C4SNetOut( hnd, 1, result, T+1);
  return( hnd);
}

