#include <solveOneLevel.h>
#include <cwrapper.h>
#include <sacinterface.h>

void *solveOneLevel( void *hnd, void *ptr_1, void *ptr_2)
{
  SACarg *sac_board, *sac_opts;

  
  sudoku__solveOneLevel2( &sac_board, &sac_opts, ptr_1, ptr_2);

  if( SACARGgetDim( sac_opts) == 0) {
    SAC2SNet_outRaw( hnd, 2, sac_board, 0);
  }
  else {
    SAC2SNet_outRaw( hnd, 1, sac_board, sac_opts);
  }

  return( hnd);
}
