#include <GenSubKeys.h>
#include <simpleDesboxes.h>
#include <sacinterface.h>

void *GenSubKeys( void *hnd, void *ptr_1)
{
  SACarg *res1, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  simpleDesboxes__genSubKeys2( &res1,  handle, ptr_1);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res1);
  
  return( hnd);
}
