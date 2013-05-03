#include "init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"

void *init( void *hnd, c4snet_data_t *array, int C_width, int C_height, int last)
{
  int *a = C4SNetGetData(array);
  int *C;

  c4snet_data_t *new = C4SNetAlloc(CTYPE_int, C_width * C_height, (void **) &C);

  memcpy(C, a, sizeof(int) * C_width * (last + 1));

  C4SNetOut( hnd, 1, new, C_width, C_height, last);
  C4SNetFree(array);

  return hnd;
}
