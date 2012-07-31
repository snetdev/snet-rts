#include <C4SNet.h>

void *gen( void *hnd , c4snet_data_t *n, c4snet_data_t *w)
{
  int int_n = *(int *) C4SNetGetData(n);
  int int_w = *(int *) C4SNetGetData(w);

  C4SNetFree(n);
  C4SNetFree(w);

  for (int i = 0; i < int_n; ++i)
     C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_n), C4SNetCreate(CTYPE_int, 1, &int_w));

  return hnd;
}
