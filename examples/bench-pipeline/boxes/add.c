#include <C4SNet.h>

void *add( void *hnd, c4snet_data_t *n, c4snet_data_t *w)
{
  int int_n = *(int *) C4SNetGetData(n);
  int int_w = *(int *) C4SNetGetData(w);

  C4SNetFree(n);
  C4SNetFree(w);

  int i, s;
  for (s = int_n, i = 0; i <= int_w; ++i)
  {
         s += int_n;
        __asm__ __volatile__ ("");
  }

  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_n), C4SNetCreate(CTYPE_int, 1, &int_w));

  return hnd;
}

void *addt( void *hnd, int n, int w)
{
  int i, s;
  for (s = n, i = 0; i <= w; ++i)
  {
         s += n;
        __asm__ __volatile__ ("");
  }

  C4SNetOut(hnd, 1, n, w);

  return hnd;
}
