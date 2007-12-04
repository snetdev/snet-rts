#include <solveOneLevel.h>
#include <cwrapper.h>
#include <sacinterface.h>

void *solveOneLevel( void *hnd, void *ptr_1, void *ptr_2)
{
  SACarg *res, *handle;

  handle = SACARGconvertFromVoidPointer( SACTYPE_SNet_SNet, hnd);
  sudoku__solveOneLevel3( &res, handle, ptr_1, ptr_2);
  SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, res);
  
  return( hnd);
}
