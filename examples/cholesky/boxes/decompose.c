#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "cholesky_prim.h"

void *
decompose (void *hnd, c4snet_data_t * a, int a_size, int bs)
{
  int p;
  int i, j;
  tile *atiles, *ltiles;
  struct timeval *tv;
  if (bs <= 0)
    {
      fprintf (stderr, "A block size must be greater than 0\n");
      exit (0);
    }
  if (a_size <= 0)
    {
      fprintf (stderr, "The dimension of matrix must be greater than 0\n");
      exit (0);
    }
  p = a_size / bs;

  tv = (struct timeval*) malloc (sizeof (struct timeval));
  gettimeofday (tv, 0);
  atiles = (tile *) malloc (sizeof (tile) * p * p);
  ltiles = (tile *) malloc (sizeof (tile) * p * p);
  memset (atiles, 0, sizeof (tile) * p * p);
  memset (ltiles, 0, sizeof (tile) * p * p);
  double *array = C4SNetGetData (a);
  for (i = 0; i < p; i++)
    for (j = 0; j <= i; j++)
      {
	atiles[i * p + j] = (double *) malloc (sizeof (double) * bs * bs);
	ltiles[i * p + j] = (double *) malloc (sizeof (double) * bs * bs);
	int ai, aj, ti, tj;
	for (ai = i * bs, ti = 0; ti < bs; ai++, ti++)
	  for (aj = j * bs, tj = 0; tj < bs; aj++, tj++)
	    atiles[i * p + j][ti * bs + tj] = array[ai * a_size + aj];
      }
  C4SNetOut (hnd, 1, C4SNetCreate (CTYPE_char, sizeof (void *), &atiles),
	     C4SNetCreate (CTYPE_char, sizeof (void *), &ltiles), bs, p, 0,
	     C4SNetCreate (CTYPE_char, sizeof (void *), &tv));
  C4SNetFree (a);
  return hnd;
}
