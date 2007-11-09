#include <SubKey.h>
#include <cwrapper.h>
#include <sacinterface.h>

void *SubKey( void *hnd, void *ptr_1, int tag)
{
  SACarg *sac_res1, *sac_res2, *sac_res3;
  

  desboxes__SubKey2( &sac_res1, 
                     &sac_res2, 
                     &sac_res3, 
                     (SACarg*)ptr_1, 
                     SACARGconvertFromIntScalar( tag));

  // this is cheating: tag is returned by sacfun
  // and should be properly converted...
  SACARGconvertToIntArray( sac_res3);
  SAC2SNet_outRaw( hnd, 1, sac_res1, sac_res2, tag);
  

  return( hnd);
}
