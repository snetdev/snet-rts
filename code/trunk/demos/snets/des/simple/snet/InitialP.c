#include <InitialP.h>
#include <simpleDesboxes.h>
#include <sacinterface.h>

void *InitialP( void *hnd, void *ptr_1)
{
  SACarg *res1, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  simpleDesboxes__InitialP2( &res1,  handle, ptr_1);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res1);
  
  return( hnd);
}
