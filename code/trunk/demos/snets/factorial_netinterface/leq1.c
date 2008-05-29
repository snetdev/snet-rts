#include <leq1.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>
#include <stdio.h>

void *leq1( void *hnd, C_Data *x)
{
  bool *bool_p;
  int int_x;

  C_Data *result;

  bool_p = malloc( sizeof( bool));

  int_x= *(int*)C2SNet_cdataGetData( x);
  
  *bool_p = (int_x <= 1);

  result = C2SNet_cdataCreate( bool_p, &C2SNetFree, &C2SNetCopyInt, &C2SNetSerializeInt);

  
  C2SNet_out( hnd, 1, x, result);
  return( hnd);
}
