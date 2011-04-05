#include <SubKey.h>
#include "desboxes.h"
#include <sacinterface.h>

void *SubKey( void *hnd, void *ptr_1, int tag)
{
  SACarg *sac_res1, *sac_res2, *sac_res3;
  int c_tag; 

  desboxes__SubKey2( &sac_res1, 
                     &sac_res2, 
                     &sac_res3, 
                     ptr_1, 
                     SACARGconvertFromIntScalar( tag));

  c_tag = SACARGconvertToIntArray( sac_res3)[0];
  
  SAC4SNet_out( hnd, 1, sac_res1, sac_res2, c_tag);

  return( hnd);
}
