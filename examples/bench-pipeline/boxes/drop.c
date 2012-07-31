#include <C4SNet.h>

void *drop( void *hnd, c4snet_data_t *n, c4snet_data_t *w, c4snet_data_t *c)
{
  C4SNetFree(n);
  C4SNetFree(w);
  C4SNetFree(c);

  return hnd;
}

#include <C4SNet.h>

void *dropt( void *hnd, int n, int w, int c)
{
  return hnd;
}
