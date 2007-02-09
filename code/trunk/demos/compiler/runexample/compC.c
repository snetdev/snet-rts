#include "compC.h"
#include "snetentities.h"

void compC(snet_handle_t *hnd, void *f_C)
{
  SNetOutRaw(hnd, 1, f_C);
}

