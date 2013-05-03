#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <C4SNet.h>
#include "compute.h"


void *compute(void *hnd, c4snet_data_t *A, c4snet_data_t *B)
{
  c4snet_data_t *b;

  b = C4SNetGetData(B);

  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, b));

  // C4SNetFree(b);

  return hnd;
}
