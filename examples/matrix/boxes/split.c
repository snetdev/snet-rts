#include "split.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"


void *split( void *hnd, c4snet_data_t *A, int A_width, int A_height, int nodes)
{
  int i, first, last, block;

  int *new_array;
  c4snet_data_t *new_data;
  int *array = C4SNetGetData(A);

  block = A_height / nodes;
  first = 0;
  last = first + block - 1;

  for (i = 0; i < nodes - 1; i++) {
    new_data = C4SNetAlloc(CTYPE_int, A_width * (last - first + 1), (void **) &new_array);

    memcpy(new_array, (array + A_width * first), sizeof(int) * A_width * (last - first + 1));

    C4SNetOut( hnd, 1, new_data, A_width, A_height, i, first, last);

    first = last + 1;
    last  += block;
  }

  last = A_height - 1;

  new_data = C4SNetAlloc(CTYPE_int, A_width * (last - first + 1), (void **) &new_array);

  memcpy(new_array, (array + A_width * first), sizeof(int) * A_width * (last - first + 1));

  C4SNetOut(hnd, 1, new_data, A_width, A_height, i, first, last);
  C4SNetFree(A);

  return hnd;
}
