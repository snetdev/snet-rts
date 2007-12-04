#include <A.h>
#include <cwrapper.h>

void A( snet_handle_t *h, void *a)
{
  SACarg *res, *handle;

  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, h);

  mini__A2( &res, handle, a);

  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res);
}
