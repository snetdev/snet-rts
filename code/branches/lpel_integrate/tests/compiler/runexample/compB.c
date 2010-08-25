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

#include "compB.h"
#include "snetentities.h"

void compB(snet_handle_t *hnd, void *f_B)
{
  SNetOutRaw(hnd, 1, f_B);
}

