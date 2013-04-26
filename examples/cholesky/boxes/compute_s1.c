#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "cholesky_prim.h"

void *
compute_s1 (void *hnd, c4snet_data_t * atiles, c4snet_data_t * ltiles, int bs,
	    int p, int k, c4snet_data_t * timestruct)
{
  int k_b, j_b, j, i_b, j_bb;
  tile restrict * restrict a_tiles = *((tile **) C4SNetGetData (atiles));
  tile restrict * restrict l_tiles = *((tile **) C4SNetGetData (ltiles));
  
  for (k_b = 0; k_b < bs; k_b++)
    {
      /*
      if (!k && (a_tiles[0][k_b * bs + k_b] <= 0))
	{
	  fprintf (stderr,
		   "Not a symmetric positive definite (SPD) matrix\n");
	  exit (0);
	}
      */

      l_tiles[k * p + k][k_b * bs + k_b] =
				    sqrt (a_tiles[k * p + k][k_b * bs + k_b]);

      for (j_b = k_b + 1; j_b < bs; j_b++)
	l_tiles[k * p + k][j_b * bs + k_b] = a_tiles[k * p + k][j_b * bs + k_b] 
					  / l_tiles[k * p + k][k_b * bs + k_b];

      for (j_bb = k_b + 1; j_bb < bs; j_bb++)
	for (i_b = j_bb; i_b < bs; i_b++)
	  a_tiles[k * p + k][i_b * bs + j_bb] =
					  a_tiles[k * p + k][i_b * bs + j_bb]
					- l_tiles[k * p + k][i_b * bs + k_b]
					* l_tiles[k * p + k][j_bb * bs + k_b];
    }
  if (k + 1 < p)
    for (j = k + 1; j < p; j++)
      C4SNetOut (hnd, 1, atiles, ltiles, bs, p, k, j, timestruct);
  else
    {
      int i, j;
      C4SNetOut (hnd, 2, ltiles, bs, p, timestruct);
      /* deallocate a_tiles array and remove atiles field.  */
      for (i = 0; i < p; i++)
	for (j = 0; j <= i; j++)
	  free (a_tiles[i * p + j]);
      free (a_tiles);
      C4SNetFree (atiles);
    }
  return hnd;
}
