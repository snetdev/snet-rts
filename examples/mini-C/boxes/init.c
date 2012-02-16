#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

#include "init.h"

#define X 6
#define Y 7

void *init( void *hnd)
{
  int i;
  int* int_x;
  c4snet_data_t *result;
   
  int_x = malloc( X * Y * sizeof( int));
  for( i=0; i<X*Y; i++) {
    int_x[i] = i;
  }
    
  result = C4SNetDataCreateArray( CTYPE_int, X*Y, int_x);

  C4SNetOut( hnd, 1, result);
  return( hnd);
}

