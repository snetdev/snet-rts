#include "init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"

void *init( void *hnd, c4snet_data_t *array, int C_width, int C_height, int last)
{
  int *a;
  int *C;

  c4snet_data_t *new;


  a = (int *)C4SNetDataGetData(array);
  
  C = SNetMemAlloc(sizeof(int) * C_width * C_height);

  memcpy(C, a, sizeof(int) * C_width * (last + 1));

  new = C4SNetDataCreateArray(CTYPE_int, C_width * C_height, C);

  C4SNetOut( hnd, 1, new, C_width, C_height, last);
  
  C4SNetDataFree(array);

  return( hnd);
}
