#include <leq1.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *leq1( void *hnd, C_Data *x)
{
  bool bool_p;
  int int_x;
  C_Data *result;

  int_x= *(int *)C4SNet_cdataGetData( x);
  
  bool_p = (int_x <= 1);

  result = C4SNet_cdataCreate( CTYPE_int, &bool_p);

  C4SNet_out( hnd, 1, x, result);
  return( hnd);
}
