#include <sub.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *sub( void *hnd, C4SNet_data_t *x)
{
  int int_x;
  c4snet_container_t *c;
  C4SNet_data_t *result;

  int_x= *(int *)C4SNet_cdataGetData( x);
  int_x -= 1;

  result = C4SNet_cdataCreate( CTYPE_int, &int_x);

  c = C4SNet_containerCreate( hnd, 1);
  C4SNet_containerSetField( c, result);

  C4SNet_outCompound( c);
  return( hnd);
}


