#include "boxB.h"

#include <stdio.h>
#include <math.h>

void *boxB( void *hnd, int a)
{
  double i, max;

  max = pow( 2.0, (double)a);
  

  for( i=0.0; i<max; i++);
  printf("\nB: Iterations: %f\n", i);    

  C4SNetOut( hnd, 1, a);
  return( hnd);

}

