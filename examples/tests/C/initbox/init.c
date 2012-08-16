#include "init.h"

#define NUM_REC 5

void *init(void *hnd)
{
  int i;
  for (i = 0; i < NUM_REC; ++i) C4SNetOut(hnd, 1, i);

  return hnd;
}
