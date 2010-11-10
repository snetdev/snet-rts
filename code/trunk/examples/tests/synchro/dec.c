#include <dec.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *dec( void *hnd, c4snet_data_t *a, c4snet_data_t *b)
{
  int int_a, int_b;
  c4snet_data_t *result;

  int_a= *(int *)C4SNetDataGetData( a);
  int_b= *(int *)C4SNetDataGetData( b);
  int_a -= int_b;

  result = C4SNetDataCreate( CTYPE_int, &int_a);

  C4SNetDataFree(a);
  C4SNetDataFree(b);

  C4SNetOut( hnd, 1, result);
  return( hnd);
}

