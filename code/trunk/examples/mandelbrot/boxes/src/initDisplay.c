#include "SDLsac.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

SDL_Thread *SDLsac_eventhandler = NULL;
SDL_mutex *SDLsac_mutex = NULL;
SDL_TimerID SDLsac_timer = NULL;

bool SDLsac_isasync;

static
void updateScreen( SDL_Surface  *surface)
{
  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexP( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to lock the access mutex");
  }

  SDL_UpdateRect( surface, 0, 0, 0, 0);

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexV( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to unlock the access mutex");
  }
}

static 
int EventHandler( void *data)
{
  SDL_Event event;

  while ( 1) {
    if (SDL_WaitEvent( &event) == 1) {
      switch (event.type) {
        case SDL_QUIT:
          exit(10);
          break;
        case SDL_USEREVENT:
          updateScreen( (SDL_Surface *) event.user.data1);
          break;

        default:
          break;
      }
    }
  }
}

static
Uint32 TimerHandler(Uint32 interval, void *param) {
  SDL_Event event;
  SDL_UserEvent userevent;

  userevent.type = SDL_USEREVENT;
  userevent.code = 0;
  userevent.data1 = param;
  userevent.data2 = NULL;

  event.type = SDL_USEREVENT;
  event.user = userevent;

  SDL_PushEvent(&event);
  return(interval);
}


Display initDisplay( int x, int y,
                     SAC_ND_PARAM_in( async_nt, bool))
{
  Display disp = NULL;

  if (SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    SAC_RuntimeError( "Failed to init SDL System: %s", SDL_GetError());
  }
#if SDL_QUIT_NICE  
  atexit( SDL_Quit);
#endif
  disp = 
    SDL_SetVideoMode(  y, 
                      x, 
                      32, SDL_HWSURFACE | SDL_ASYNCBLIT );

  if ( disp == NULL) {
    SAC_RuntimeError( "Failed to init SDL Display: %s", SDL_GetError());
  }

  SDL_WM_SetCaption( "SaC SDL Output", NULL);

  SDLsac_isasync = SAC_ND_A_FIELD( async_nt);

  if( SDLsac_isasync) {
    /*
     * register an event handler 
     */ 
    SDLsac_eventhandler = SDL_CreateThread( EventHandler, NULL);
  
    /*
     * start a display update timer to update 20 times a second
     */
    SDLsac_timer = SDL_AddTimer( 500, TimerHandler, disp);
    if ( SDLsac_timer == NULL) {
      SAC_RuntimeError( "Failed to init update timer");
    }
  } 

  /*
   * and a shiny mutex as well
   */
  SDLsac_mutex = SDL_CreateMutex();
  
  return( disp);
}
