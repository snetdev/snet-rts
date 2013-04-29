#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cholesky_prim.h"

void *
distribute (void *hnd, c4snet_data_t * fatiles, c4snet_data_t * fltiles,
		 int bs, int p, int k, c4snet_data_t * timestruct)
{
  int i, j;
  for (j = k + 1; j < p; j++)
    for (i = k + 1; i <= j; i++)
        C4SNetOut (hnd, 1, fatiles, fltiles, bs, p, k, j * p + i, timestruct);
  return hnd;
}

void *
update (void *hnd, c4snet_data_t * fatiles, c4snet_data_t * fltiles, int bs,
	int p, int k, int index, c4snet_data_t *timestruct)
{
  tile restrict * restrict a_tiles = *((tile **) C4SNetGetData (fatiles));
  tile restrict * restrict l_tiles = *((tile **) C4SNetGetData (fltiles));
  int j_b, k_b, i_b;
  int i = index % p, j = index / p;
  tile l1_block, l2_block;
      // Diagonal tile
      if (i == j)
	l2_block = l_tiles[j * p + k];
      else
	{
	  l2_block = l_tiles[i * p + k];
	  l1_block = l_tiles[j * p + k];
	}

      for (j_b = 0; j_b < bs; j_b++)
	for (k_b = 0; k_b < bs; k_b++)
	  {
	    double temp = -1 * l2_block[j_b * bs + k_b];
	    if (i != j)
	      for (i_b = 0; i_b < bs; i_b++)
		a_tiles[j * p + i][i_b * bs + j_b] =
			      a_tiles[j * p + i][i_b * bs + j_b]
			    + temp * l1_block[i_b * bs + k_b];
	    else
	      for (i_b = j_b; i_b < bs; i_b++)
		a_tiles[j * p + i][i_b * bs + j_b] =
				    a_tiles[j * p + i][i_b * bs + j_b]
				  + temp * l2_block[i_b * bs + k_b];
	  }
  C4SNetOut (hnd, 1, fatiles, fltiles, bs, p, k, timestruct);
  return hnd;
}
