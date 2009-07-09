#include "multiply.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"

void *multiply( void *hnd, c4snet_data_t *a, c4snet_data_t *B, int A_width, int A_height, int B_width, int B_height, int first, int last)
{
  int *array_a;
  int *array_B;
  int *c;

  int i, j,l;

  c4snet_data_t *new;

  array_a = (int *)C4SNetDataGetData(a);
  array_B = (int *)C4SNetDataGetData(B);

  c = SNetMemAlloc(sizeof(int) * B_width * (last - first + 1));

  memset(c, 0, B_width * (last - first + 1) * sizeof(int));

  

  for(i = 0; i < last - first + 1; i++) { // For each row
    for(j = 0; j < B_width; j++) {        // For each element of the row
      for(l = 0; l < A_width; l++) {
	(c + i * B_width)[j] += (array_a + i * A_width)[l] * (array_B + l * B_width)[j];
      }
    }
  }

  new = C4SNetDataCreateArray(CTYPE_int, B_width * (last - first + 1), c);
  C4SNetOut( hnd, 1, new, B_width, A_height, first, last);
  
  C4SNetDataFree(a);
  C4SNetDataFree(B);

  return( hnd);
}
