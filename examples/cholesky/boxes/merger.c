#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "cholesky_prim.h"

void *
gen_counter (void *hnd)
{
  C4SNetOut (hnd, 1, 0);
  return hnd;
}

void *
merge (void *hnd, c4snet_data_t * atiles, c4snet_data_t * ltiles, int counter,
       int bs, int p, int k, c4snet_data_t *timestruct)
{
  if (++counter < p - k - 1)
    C4SNetOut (hnd, 1, counter);
  else
    C4SNetOut (hnd, 2, atiles, ltiles, bs, p, k, timestruct);
  return hnd;
}

void *
finalize (void *hnd, c4snet_data_t * fltiles, int bs, int p, 
	  c4snet_data_t * timestruct)
{
  tile restrict * restrict tiles = *((tile **) C4SNetGetData (fltiles));
  struct timeval *tv = *((struct timeval **) C4SNetGetData (timestruct));
  struct timeval tv2;
  gettimeofday(&tv2, 0);
  long elapsed = (tv2.tv_sec-tv->tv_sec)*1000000 + tv2.tv_usec-tv->tv_usec;
  double time_result = (double) elapsed / 1000000;
  free (tv);
  int size = p * bs;
  double *array = (double *) malloc (sizeof (double) * size * size);
  memset (array, 0.0, sizeof (double) * sizeof (size * size));
  int i, j;
  for (i = 0; i < p; i++)
    {
      for (j = 0; j <= i; j++)
		{
		  int k;
		  for (k = 0; k < bs * bs; k++)
			{
			  int px = j * bs + (k % bs);
			  int py = i * bs + (k / bs);
			  array[py * size + px] = tiles[i * p + j][k];
			}
		  free (tiles[i * p + j]);
		}
    }
  free (tiles);
  C4SNetOut (hnd, 1, C4SNetCreate (CTYPE_double, size * size, array),
					 C4SNetCreate (CTYPE_double, 1, &time_result));
  free (array);
  C4SNetFree (fltiles);
  C4SNetFree (timestruct);
  return hnd;
}
