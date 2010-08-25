#include <leq1.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *leq1( void *hnd, c4snet_data_t *x)
{
  bool bool_p;
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetDataGetData( x);
  
  bool_p = (int_x <= 1);

  result = C4SNetDataCreate( CTYPE_int, &bool_p);

  C4SNetOut( hnd, 1, x, result);

  return( hnd);
}
