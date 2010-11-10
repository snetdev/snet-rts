#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

#include "foo.h"

void *foo( void *hnd, c4snet_data_t *x, int counter)
{
  int int_x;
  c4snet_data_t *result;

  int_x= *(int *)C4SNetDataGetData( x);

  if (int_x > 0) {
    int_x -= 1;
    /* emit {A, <cnt>} record */
    result = C4SNetDataCreate( CTYPE_int, &int_x);
    C4SNetOut( hnd, 1, result, counter+1);
  } else {
    /* emit {B} record */
    result = C4SNetDataCreate( CTYPE_int, &counter);
    C4SNetOut( hnd, 2, result);
  }
  C4SNetDataFree(x);
  return( hnd);
}

