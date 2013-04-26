#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cholesky_prim.h"

void *
sync (void *hnd, c4snet_data_t * fatiles, c4snet_data_t * fltiles,
      int counter, int bs, int p, int k, c4snet_data_t *timestruct)
{
  if (++counter < (p - k) * (p - k - 1) / 2)
    C4SNetOut (hnd, 1, counter);
  else
    C4SNetOut (hnd, 2, fatiles, fltiles, bs, p, k, timestruct);
  return hnd;
}
