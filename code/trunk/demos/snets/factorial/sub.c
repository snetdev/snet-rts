#include <sub.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *sub( void *hnd, c4snet_data_t *x)
{
  int int_x;
  c4snet_container_t *c;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetDataGetData( x);
  int_x -= 1;

  result = C4SNetDataCreate( CTYPE_int, &int_x);

  c = C4SNetContainerCreate( hnd, 1);
  C4SNetContainerSetField( c, result);

  C4SNetDataFree(x);

  C4SNetContainerOut( c);
  return( hnd);
}


