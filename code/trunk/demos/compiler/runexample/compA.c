#include "compA.h"
#include "snetentities.h"

void compA(snet_handle_t *hnd, void *f_A)
{
  SNetOutRaw(hnd, 1, f_A);
}

