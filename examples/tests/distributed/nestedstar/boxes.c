#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "C4SNet.h"

extern int node_location;

void* tell(void *hnd, int T, int S, int R)
{
  fflush(stdout);
  usleep(50 * 1000);
  printf("%s @ %d: T %d, S %d, R %d\n", __func__, node_location, T, S, R);
  fflush(stdout);
  usleep(50 * 1000);
  C4SNetOut(hnd, 1, T, S, R);
  return hnd;
}

