/*****************************************************************
 *
 * Box source file of run-example from SNet compiler implementation guide.
 * Just for testing compilation/linking of compiler.
 *
 * Author: Kari Keinanen, VTT Technical Research Centre of Finland
 *
 * Date:   21.02.2007
 * 
 ****************************************************************/

#include "compC.h"
#include "snetentities.h"

void compC(snet_handle_t *hnd, void *f_C)
{
  SNetOutRaw(hnd, 1, f_C);
}

