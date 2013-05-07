#include "merger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"

void *merger(void *hnd, c4snet_data_t *C, c4snet_data_t *c, int C_width,
             int C_height, int first, int last)
{
  int *array = C4SNetGetData(C);
  int *row = C4SNetGetData(c);

  c4snet_data_t *new;

  memcpy(array + first * C_width, row, sizeof(int) * C_width * (last - first + 1));

  new = C4SNetCreate(CTYPE_int, C_width * C_height, array);

  C4SNetOut(hnd, 1, new, C_width, C_height, first, last);
  C4SNetFree(c);
  C4SNetFree(C);

  return hnd;
}
