#include "examine.h"
#include "snetentities.h"

void examine(snet_handle_t *hnd, void *f_P, void *f_Q)
{
  SNetOutRaw(hnd, 2, f_P, f_Q);
}

