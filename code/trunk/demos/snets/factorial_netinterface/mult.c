#include <mult.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>
#include <stdio.h>

void *mult( void *hnd, C_Data *x, C_Data *r)
{ 
  C_Data *result;
  int int_x, int_r, *int_rr;

  int_rr = malloc( sizeof( int));

  int_x = *(int*) C2SNet_cdataGetData( x);
  int_r = *(int*) C2SNet_cdataGetData( r);
  *int_rr = int_x * int_r;

  result = C2SNet_cdataCreate( int_rr, &C2SNetFree, &C2SNetCopyInt, &C2SNetSerializeInt);

  C2SNet_out( hnd, 1, result);
  return( hnd);
}
