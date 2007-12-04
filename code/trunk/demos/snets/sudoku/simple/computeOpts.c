#include <computeOpts.h>
#include <cwrapper.h>

void *computeOpts( void *hnd, void *ptr)
{
  SACarg *res, *handle;
  
  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  sudoku__computeOpts2( &res, handle, ptr);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res);  

  return( hnd);
}
