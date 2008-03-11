#include <InitialP.h>
#include <cwrapper.h>
#include <sacinterface.h>
#include <stdio.h>
void *InitialP( void *hnd, void *ptr_1)
{
  SACarg *sac_res1, *sac_res2;

  desboxes__InitialP1( &sac_res1, &sac_res2, ptr_1);
  SAC2SNet_outRaw( hnd, 1, sac_res1, sac_res2);
  

  return( hnd);
}
