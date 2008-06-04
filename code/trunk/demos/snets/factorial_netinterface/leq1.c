#include <leq1.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *leq1( void *hnd, C_Data *x)
{
  bool *bool_p;
  int int_x;

  C_Data *result;

  bool_p = malloc( sizeof( bool));

  int_x= *(int*)C4SNet_cdataGetData( x);
  
  *bool_p = (int_x <= 1);

  result = C4SNet_cdataCreate( bool_p, &C4SNetFree, &C4SNetCopyInt, 
			       &C4SNetSerializeInt, &C4SNetEncodeInt);

  
  C4SNet_out( hnd, 1, x, result);
  return( hnd);
}
