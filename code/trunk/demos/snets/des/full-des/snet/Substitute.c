#include <Substitute.h>
#include "desboxes.h"
#include <sacinterface.h>

void *Substitute( void *hnd, void *ptr_1)
{
  SACarg *sac_result;

 desboxes__Substitute1( &sac_result, ptr_1);

  SAC2SNet_out( hnd, 1, sac_result);
  

  return( hnd);
}
