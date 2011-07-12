#include <stdlib.h>
#include <assert.h>
#include <bool.h>
#include <stdio.h>

#include "foo.h"

void *foo( void *hnd, c4snet_data_t *x, int children, int counter)
{
  int int_x;
  c4snet_data_t *result;

  assert( children >= 0);

  int_x= *(int *)C4SNetDataGetData( x);

  if (children > 0) {
    int i;
    for (i=0; i<children; i++) {
      /* emit {A, <R>, <cnt>} record */
      result = C4SNetDataCreate( CTYPE_int, &int_x);
      C4SNetOut( hnd, 1, result, 0, 0);
    }
  } else {
    if (int_x > 0) {
      int_x -= 1;
      /* emit {A, <R>, <cnt>} record */
      result = C4SNetDataCreate( CTYPE_int, &int_x);
      C4SNetOut( hnd, 1, result, 0, counter+1);
    } else {
      /* emit {B} record */
      result = C4SNetDataCreate( CTYPE_int, &counter);
      C4SNetOut( hnd, 2, result);
    }
  }
  C4SNetDataFree(x);
  return( hnd);
}

