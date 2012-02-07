#include "bool.h"
#include "mult.h"

void *mult( void *hnd, c4snet_data_t *x, c4snet_data_t *r)
{
  c4snet_data_t *result;
  int int_x, int_r, int_rr ;

  int_x = *(int *)C4SNetDataGetData( x);
  int_r = *(int *)C4SNetDataGetData( r);
  int_rr = int_x * int_r;

  result = C4SNetDataCreate( CTYPE_int, &int_rr);

  C4SNetDataFree(x);
  C4SNetDataFree(r);

  C4SNetOut( hnd, 1, result);
  return( hnd);
}
