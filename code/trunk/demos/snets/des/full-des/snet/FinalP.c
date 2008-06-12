#include <FinalP.h>
#include "desboxes.h"
#include <sacinterface.h>

void *FinalP( void *hnd, void *ptr_1, void *ptr_2)
{
  SACarg *sac_result;

  desboxes__FinalP2( &sac_result, ptr_1, ptr_2);

  SAC2SNet_out( hnd, 1, sac_result);
  

  return( hnd);
}
