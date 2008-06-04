#include <sub.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *sub( void *hnd, C_Data *x)
{
  int *int_x;
  c4snet_container_t *c;
  C_Data *result;

  int_x = malloc( sizeof( int));

  *int_x= *(int*)C4SNet_cdataGetData( x);
  *int_x -= 1;

  result = C4SNet_cdataCreate( int_x, &C4SNetFree, &C4SNetCopyInt,  
			       &C4SNetSerializeInt, &C4SNetEncodeInt);

  c = C4SNet_containerCreate( hnd, 1);
  C4SNet_containerSetField( c, result);

  C4SNet_outCompound( c);
  return( hnd);
}


