#include <mult.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *mult( void *hnd, C4SNet_data_t *x, C4SNet_data_t *r)
{ 
  C4SNet_data_t *result;
  int int_x, int_r, int_rr ;

  int_x = *(int *)C4SNet_cdataGetData( x);
  int_r = *(int *)C4SNet_cdataGetData( r);
  int_rr = int_x * int_r;

  result = C4SNet_cdataCreate( CTYPE_int, &int_rr);

  C4SNetFree(x);
  C4SNetFree(r);

  C4SNet_out( hnd, 1, result);
  return( hnd);
}
