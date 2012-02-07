#include "split.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"


void *split( void *hnd, c4snet_data_t *A, int A_width, int A_height, int nodes)
{
  int *array;
  int i;

  int *new_array;
  c4snet_data_t *new_data;

  int first, last, block;


  array = (int *)C4SNetDataGetData(A);

  block = A_height / nodes;

  first = 0;
  last = first + block - 1;

  for(i = 0; i < nodes - 1; i++) {
    new_array = (int *)SNetMemAlloc(sizeof(int) * A_width * (last - first + 1)); 

    memcpy(new_array, (array + A_width * first), sizeof(int) * A_width * (last - first + 1));

    new_data = C4SNetDataCreateArray(CTYPE_int, A_width * (last - first + 1), new_array);

    C4SNetOut( hnd, 1, new_data, A_width, A_height, i, first, last);

    first = last + 1;
    last  += block;
  }

  last = A_height - 1;

  new_array = (int *)SNetMemAlloc(sizeof(int) * A_width * (last - first + 1)); 

  memcpy(new_array, (array + A_width * first), sizeof(int) * A_width * (last - first + 1));

  new_data = C4SNetDataCreateArray(CTYPE_int, A_width * (last - first + 1), new_array);

  C4SNetOut( hnd, 1, new_data, A_width, A_height, i, first, last);

  C4SNetDataFree(A);

  return( hnd);
}
