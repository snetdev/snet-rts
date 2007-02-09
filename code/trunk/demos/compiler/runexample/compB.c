#include "compB.h"
#include "snetentities.h"

void compB(snet_handle_t *hnd, void *f_B)
{
  SNetOutRaw(hnd, 1, f_B);
}

