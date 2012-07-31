#include <C4SNet.h>

void *drop( void *hnd, c4snet_data_t *n, c4snet_data_t *w)
{
  C4SNetFree(n);
  C4SNetFree(w);

  return hnd;
}
