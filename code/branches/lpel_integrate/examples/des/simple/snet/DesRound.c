#include <DesRound.h>
#include <simpleDesboxes.h>
#include <sacinterface.h>

void *DesRound( void *hnd, void *ptr_1, void *ptr_2, void *ptr_3, int tag_2)
{
  SACarg *res1, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  simpleDesboxes__DesRound5( &res1,  handle, ptr_1, ptr_2, 
                             ptr_3, 
                             SACARGconvertFromIntScalar( tag_2));
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res1);
  
  return( hnd);
}
