#include <init.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>


#define X 6
#define Y 7

void *init( void *hnd)
{
  int i;
  int* int_x;
  c4snet_container_t *c;
  c4snet_data_t *result;
   
  int_x = SNetMemAlloc( X * Y * sizeof( int));
  for( i=0; i<X*Y; i++) {
    int_x[i] = i;
  }
    
  result = C4SNetDataCreateArray( CTYPE_int, X*Y, int_x);

  c = C4SNetContainerCreate( hnd, 1);
  C4SNetContainerSetField( c, result);

  C4SNetContainerOut( c);
  return( hnd);
}

