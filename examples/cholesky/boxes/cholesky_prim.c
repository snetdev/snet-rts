#include <stdio.h>
#include "cholesky_prim.h"

inline int get (int i, int j, int kx, int ky, int bs, int size)
{
  int k = kx * bs + ky;
  int px = j * bs + (k % bs);
  int py = i * bs + (k / bs);
  return py * size + px;
}
