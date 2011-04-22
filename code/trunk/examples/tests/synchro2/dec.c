#include <dec.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *dec( void *hnd, c4snet_data_t *state, c4snet_data_t *inval)
{
  int int_a, int_b;
  c4snet_data_t *resultA, *resultB;

  int_a= *(int *)C4SNetDataGetData(state);
  int_b = *(int *) C4SNetDataGetData(inval);

  if (int_a != 0) {
    resultA = C4SNetDataCreate( CTYPE_int, &int_b);
    C4SNetOut( hnd, 1, resultA);
  } else {
    resultB = C4SNetDataCreate( CTYPE_int, &int_b);
    C4SNetOut( hnd, 2, resultB);
 }
 
  C4SNetDataFree(state);
  C4SNetDataFree(inval);
  return( hnd);
}

/*
void *dec( void *hnd, c4snet_data_t *b)
{
  int int_b;
  c4snet_data_t *resultB;

  int_b= *(int *)C4SNetDataGetData(b);
  int_b -= 1;

  resultB = C4SNetDataCreate( CTYPE_int, &int_b);

  C4SNetDataFree(b);

  C4SNetOut( hnd, 1, resultB);
  return( hnd);
}
*/

