#include <C4SNet.h>

void *t2f( void *hnd , int n, int w, int c)
{
  C4SNetOut(hnd, 1, 
            C4SNetCreate(CTYPE_int, 1, &n), 
            C4SNetCreate(CTYPE_int, 1, &w),
            C4SNetCreate(CTYPE_int, 1, &c));

  return hnd;
}

void *f2t( void *hnd , c4snet_data_t *n, c4snet_data_t *w, c4snet_data_t *c)
{
  int int_n = *(int *) C4SNetGetData(n);
  int int_w = *(int *) C4SNetGetData(w);
  int int_c = *(int *) C4SNetGetData(c);

  C4SNetFree(n);
  C4SNetFree(w);
  C4SNetFree(c);

  C4SNetOut(hnd, 1, int_n, int_w, int_c);

  return hnd;
}
