#include <BitExpand.h>
#include <cwrapper.h>
#include <sacinterface.h>

void *BitExpand( void *hnd, void *ptr_1)
{
  SACarg *sac_result;

 desboxes__BitExpand1( &sac_result, ptr_1);

  SAC2SNet_out( hnd, 1, sac_result);
  

  return( hnd);
}
