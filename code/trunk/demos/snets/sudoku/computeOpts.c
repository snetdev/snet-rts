#include <computeOpts.h>
#include <cwrapper.h>

void *computeOpts( void *hnd, void *ptr)
{
  SACarg *sac_board, *sac_opts;

  sudoku__computeOpts1( &sac_board, &sac_opts, ptr);
  
  SAC2SNet_outRaw( hnd, 1, sac_board, sac_opts);

  return( hnd);
}
