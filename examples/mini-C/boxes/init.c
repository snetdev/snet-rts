#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

#include "init.h"

#define X 6
#define Y 7

void *init( void *hnd)
{
  int *int_x;
  c4snet_data_t *result;

  result = C4SNetAlloc(CTYPE_int, X*Y, &int_x);
  for (int i = 0; i < X*Y; i++) int_x[i] = i;

  C4SNetOut(hnd, 1, result);
  return hnd;
}
