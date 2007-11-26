#include <A.h>
#include <cwrapper.h>
#include <stdio.h>

void A( snet_handle_t *h, void *a)
{
  SACarg *res, *handle;

  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, h);

  printf("Dim: %d\n", SACARGgetDim( a));

  mini__A2( &res, handle, a);

  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res);
}
