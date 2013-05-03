#include "init.h"
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>

void *init( void *hnd)
{
  int int_x;
  c4snet_data_t *result;

  int_x = 1111;
  result = C4SNetCreate(CTYPE_int, 1, &int_x);
  C4SNetOut( hnd, 1, result);

  return hnd;
}

