#include "merger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"

void *merger( void *hnd, c4snet_data_t *C, c4snet_data_t *c, int C_width, int C_height, int first, int last)
{
  int *array;
  int *row;

  c4snet_data_t *new;

  array = (int *)C4SNetDataGetData(C);
  row = (int *)C4SNetDataGetData(c);

  memcpy((array + first * C_width), row, sizeof(int) * C_width * (last - first + 1));

  new = C4SNetDataCreateArray(CTYPE_int, C_width * C_height, array);

  C4SNetOut( hnd, 1, new, C_width, C_height, first, last);
  
  C4SNetDataFree(c);

  return( hnd);
}
