#include <leq1.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>
#include <stdio.h>

void *leq1( void *hnd, C_Data *x)
{
  bool *bool_p;
  int *int_x;

  C_Data *result, *new_x;

  bool_p = malloc( sizeof( bool));
  int_x = malloc( sizeof( int));

  *int_x= *(int*)C4SNet_cdataGetData( x);
  
  *bool_p = (*int_x <= 1);

  result = C4SNet_cdataCreate( bool_p, &myfree, &mycopy, NULL, NULL);
  new_x = C4SNet_cdataCreate( int_x, &myfree, &mycopy, NULL, NULL);
  
  C4SNet_out( hnd, 1, new_x, result);
  return( hnd);
}
