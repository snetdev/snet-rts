#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cholesky_prim.h"

void *
solve_s2 (void *hnd, c4snet_data_t * atiles, c4snet_data_t * ltiles, int bs,
	    int p, int k, int j, c4snet_data_t * timestruct)
{
  int k_b, j_b, i_b;
  tile restrict * restrict a_tiles = *((tile **) C4SNetGetData (atiles));
  tile restrict * restrict l_tiles = *((tile **) C4SNetGetData (ltiles));
  
  memset (l_tiles[j * p + k], 0, sizeof (double) * bs * bs);
  assert (j != k);
  
  for (k_b = 0; k_b < bs; k_b++)
    {
      for (i_b = 0; i_b < bs; i_b++)
	l_tiles[j * p + k][i_b * bs + k_b] =
					a_tiles[j * p + k][i_b * bs + k_b]
				      / l_tiles[k * p + k][k_b * bs + k_b];
      for (j_b = k_b + 1; j_b < bs; j_b++)
	for (i_b = 0; i_b < bs; i_b++)
	  a_tiles[j * p + k][i_b * bs + j_b] =
					a_tiles[j * p + k][i_b * bs + j_b]
				      - l_tiles[k * p + k][j_b * bs + k_b] 
				      * l_tiles[j * p + k][i_b * bs + k_b];
    }
  C4SNetOut (hnd, 1, atiles, ltiles, bs, p, k, timestruct);
  return hnd;
}
