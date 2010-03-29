#include "SDLsac.h"

#include <unistd.h>

Display drawArrayOffset( Display disp,
                         SAC_ND_PARAM_in( ar_nt, int),
                         int pic_offset[2])
{
  int xaxis, yaxis, aroffset, screenoffset, xoffset, yoffset;

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexP( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to lock the access mutex");
  }

  xoffset = pic_offset[0];
  yoffset = pic_offset[1];

  /*
   * check bounds
   */


  if ( xoffset < 0 || yoffset < 0 ||
       (SAC_ND_A_DESC_SHAPE( ar_nt, 1)+xoffset > disp->w) ||
       (SAC_ND_A_DESC_SHAPE( ar_nt, 0)+yoffset > disp->h) ) {
    SAC_RuntimeError( "Cannot draw array of shape [ %d, %d] starting \n"
                      "***at [%d,%d] on display of size [ %d, %d] ! \n",
                      SAC_ND_A_DESC_SHAPE( ar_nt, 0),
                      SAC_ND_A_DESC_SHAPE( ar_nt, 1),
                      xoffset,
                      yoffset,
                      disp->w,
                      disp->h);
  }


  /*
   * lock the screen for drawing
   */
  if (SDL_MUSTLOCK( disp)) {
    if (SDL_LockSurface( disp) < 0) {
      SAC_RuntimeError( "Failed to lock the SDL Display");
    }
  }

  /*
   * draw
   */
  /*printf("xoffset = %i, yoffset = %i\n", xoffset, yoffset);*/

  aroffset = 0;
  for (yaxis = yoffset; 
       yaxis < SAC_ND_A_DESC_SHAPE( ar_nt, 0)+yoffset; yaxis ++) {
    
    screenoffset = yaxis * disp->pitch / 4;

    for (xaxis = xoffset; 
         xaxis < SAC_ND_A_DESC_SHAPE( ar_nt, 1)+xoffset; xaxis ++) {
      Uint32 *bptr = (Uint32 *) disp->pixels 
        + screenoffset + xaxis;

      *bptr = ( ((Uint32)SAC_ND_A_FIELD( ar_nt)[aroffset]) << 16
                | ((Uint32)SAC_ND_A_FIELD( ar_nt)[aroffset+1]) << 8
                | ((Uint32)SAC_ND_A_FIELD( ar_nt)[aroffset+2]) 
              );
      aroffset+=3;
    }
  }

  /*
   * unlock it
   */
  if (SDL_MUSTLOCK( disp)) {
    SDL_UnlockSurface( disp);
  }

  if( !SDLsac_isasync) {
    SDL_UpdateRect( disp, xoffset, yoffset, 
        SAC_ND_A_DESC_SHAPE( ar_nt, 1),
        SAC_ND_A_DESC_SHAPE( ar_nt, 0));
  }

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexV( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to unlock the access mutex");
  }

  SAC_ND_DEC_RC_FREE( ar_nt, 1, )
  return( disp);
}
           
