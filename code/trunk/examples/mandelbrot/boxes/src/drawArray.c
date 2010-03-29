#include "SDLsac.h"

#include <unistd.h>
#include <stdio.h>

Display drawArray( Display disp,
                   SAC_ND_PARAM_in( ar_nt, int))
{
  int xaxis, yaxis, aroffset, screenoffset;

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexP( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to lock the access mutex");
  }

  /*
   * check bounds
   */
  if ( (SAC_ND_A_DESC_SHAPE( ar_nt, 1) != NT_NAME( disp_nt)->w) ||
       (SAC_ND_A_DESC_SHAPE( ar_nt, 0) != NT_NAME( disp_nt)->h) ) {
    SAC_RuntimeError( "Cannot draw array of shape [ %d, %d] on display\n"
                      "*** of size [ %d, %d] ! \n",
                      SAC_ND_A_DESC_SHAPE( ar_nt, 0),
                      SAC_ND_A_DESC_SHAPE( ar_nt, 1),
                      NT_NAME( disp_nt)->w,
                      NT_NAME( disp_nt)->h);
  }
  printf("%d,%d,%d,%d\n", xaxis, yaxis, SAC_ND_A_DESC_SHAPE( ar_nt, 0),SAC_ND_A_DESC_SHAPE( ar_nt, 0));
  /*
   * lock the screen for drawing
   */
  if (SDL_MUSTLOCK( NT_NAME( disp_nt))) {
    if (SDL_LockSurface( NT_NAME( disp_nt)) < 0) {
      SAC_RuntimeError( "Failed to lock the SDL Display");
    }
  }

  /*
   * draw
   */
  aroffset = 0;
  for (yaxis = 0; yaxis < SAC_ND_A_DESC_SHAPE( ar_nt, 0); yaxis ++) {
    screenoffset = yaxis * NT_NAME( disp_nt)->pitch / 4;
    for (xaxis = 0; xaxis < SAC_ND_A_DESC_SHAPE( ar_nt, 1); xaxis ++) {
      Uint32 *bptr = (Uint32 *) NT_NAME( disp_nt)->pixels 
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
  if (SDL_MUSTLOCK( NT_NAME( disp_nt))) {
    SDL_UnlockSurface( NT_NAME( disp_nt));
  }

  if( !SDLsac_isasync) {
    SDL_UpdateRect( NT_NAME( disp_nt), 0, 0, 0, 0);
  }

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexV( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to unlock the access mutex");
  }

  SAC_ND_DEC_RC_FREE( ar_nt, 1, )
  return(disp);
}
           
