#include <sub.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>
#include <stdio.h>

void *sub( void *hnd, C_Data *x)
{
  int *int_x;
  c2snet_container_t *c;
  C_Data *result;

  int_x = malloc( sizeof( int));

  *int_x= *(int*)C2SNet_cdataGetData( x);
  *int_x -= 1;

  result = C2SNet_cdataCreate( int_x, &myfree, &mycopyInt,  &myserializeInt);

  c = C2SNet_containerCreate( hnd, 1);
  C2SNet_containerSetField( c, result);

  C2SNet_out( c);
  return( hnd);
}


