#include <FinalP.h>
#include <cwrapper.h>
#include <sacinterface.h>

void *FinalP( void *hnd, void *ptr_1, void *ptr_2)
{
  SACarg *res1, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  simpleDesboxes__FinalP3( &res1,  handle, ptr_1, ptr_2);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res1);
  
  return( hnd);
}
