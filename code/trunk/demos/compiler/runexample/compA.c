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

#include "compA.h"
#include "snetentities.h"

void compA(snet_handle_t *hnd, void *f_A)
{
  SNetOutRaw(hnd, 1, f_A);
}

