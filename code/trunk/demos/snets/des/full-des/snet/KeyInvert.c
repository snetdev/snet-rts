#include <KeyInvert.h>
#include <desboxes.h>
#include <sacinterface.h>

void *KeyInvert( void *hnd, void *ptr_1)
{
  SACarg *res1, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  desboxes__KeyInvert2( &res1,  handle, ptr_1);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res1);
  
  return( hnd);
}

