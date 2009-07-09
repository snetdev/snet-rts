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

#include "examine.h"
#include "snetentities.h"

void examine(snet_handle_t *hnd, void *f_P, void *f_Q)
{
  SNetOutRaw(hnd, 2, f_P, f_Q);
}

