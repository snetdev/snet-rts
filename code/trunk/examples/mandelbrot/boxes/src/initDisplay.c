#include "SDLsac.h"
#include <stdlib.h>
#include <unistd.h>

#define MIN_UPDATE_INTERVAL 50
#define START_UPDATE_INTERVAL 200

SDL_Thread *SDLsac_eventhandler = NULL;
SDL_mutex *SDLsac_mutex = NULL;
SDL_TimerID SDLsac_timer = NULL;

#ifdef UPDATE_VIA_SEMAPHORE
SDL_sem *SDLsac_updatesem = NULL;
SDL_Thread *SDLsac_updater = NULL;
#endif /* UPDATE_VIA_SEMAPHORE */

bool SDLsac_isasync = 0;

selmode_t SDLsac_selmode = SEL_none;
SDL_sem *SDLsac_selectsem = NULL;
int SDLsac_selection[4] = {0,0,0,0};

static
int UpdateScreen( void *surface)
{
#ifdef UPDATE_VIA_SEMAPHORE
  while (1) {
    /*
     * wait for update event 
     */
    if (SDL_SemWait( SDLsac_updatesem) == 0) {
      
#endif /* UPDATE_VIA_SEMAPHORE */
      /*
       * accessing the display needs to be mutually exclusive
       */
      if (SDL_mutexP( SDLsac_mutex)==-1){
        SAC_RuntimeError( "Failed to lock the access mutex");
      }

      SDL_Flip( (SDL_Surface *) surface);

      /*
       * accessing the display needs to be mutually exclusive
       */
      if (SDL_mutexV( SDLsac_mutex)==-1){
        SAC_RuntimeError( "Failed to unlock the access mutex");
      }
#ifdef UPDATE_VIA_SEMAPHORE
    }
  }
#endif /* UPDATE_VIA_SEMAPHORE */

  return 0;
}

static
void InvertRect( Display disp, int *pos)
{
  int yaxis, xaxis;
  int x1, x2, y1, y2;
  int screenoffset;

  if (pos[0] < pos[2]) {
    x1 = pos[0];
    x2 = pos[2];
  } else {
    x1 = pos[2];
    x2 = pos[0];
  }

  if (pos[1] < pos[3]) {
    y1 = pos[1];
    y2 = pos[3];
  } else {
    y1 = pos[3];
    y2 = pos[1];
  }

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexP( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to lock the access mutex");
  }

  /*
   * lock the screen for drawing
   */
  if (SDL_MUSTLOCK( NT_NAME( disp_nt))) {
    if (SDL_LockSurface( NT_NAME( disp_nt)) < 0) {
      SAC_RuntimeError( "Failed to lock the SDL Display");
    }
  }

  for (yaxis = y1; yaxis < y2; yaxis ++) {
    screenoffset = yaxis * disp->pitch / 4;
    for (xaxis = x1; xaxis < x2 ; xaxis ++) {
      Uint32 *bptr = (Uint32 *) disp->pixels 
        + screenoffset + xaxis;

      *bptr = ~(*bptr);
    }
  }

  /*
   * unlock it
   */
  if (SDL_MUSTLOCK( NT_NAME( disp_nt))) {
    SDL_UnlockSurface( NT_NAME( disp_nt));
  }

  /*
   * accessing the display needs to be mutually exclusive
   */
  if (SDL_mutexV( SDLsac_mutex)==-1){
    SAC_RuntimeError( "Failed to unlock the access mutex");
  }
}

static 
int EventHandler( void *screen)
{
  SDL_Event event;
  int done = 0;

  while ( !done) {
    if (SDL_WaitEvent( &event) == 1) {
      switch (event.type) {
        case SDL_QUIT:
          /* someone clicked the close button on the window */
          SDL_Quit();
          exit(10);
          break;

        case SDL_USEREVENT_DRAW:
#ifdef UPDATE_VIA_SEMAPHORE
          SDL_SemPost( SDLsac_updatesem);
#else /* UPDATE_VIA_SEMAPHORE */
          UpdateScreen( (SDL_Surface *) event.user.data1);
#endif /* UPDATE_VIA_SEMAPHORE */
          break;

        case SDL_USEREVENT_QUIT:
          done = 1;
          break;

        case SDL_MOUSEBUTTONDOWN:
          if ((event.button.button == 1) && (SDLsac_selmode == SEL_top)) {
            SDLsac_selmode = SEL_bottom;
            SDLsac_selection[0] = event.button.x;
            SDLsac_selection[1] = event.button.y;
            SDLsac_selection[2] = event.button.x;
            SDLsac_selection[3] = event.button.y;
          }
          break;

        case SDL_MOUSEBUTTONUP:
          if ((event.button.button == 1) && (SDLsac_selmode == SEL_bottom)) {
            SDLsac_selmode = SEL_none;
            SDLsac_selection[2] = event.button.x;
            SDLsac_selection[3] = event.button.y;

            SDL_SemPost( SDLsac_selectsem);
          }
          if ((event.button.button == 2) && (SDLsac_selmode != SEL_none)) {
            SDLsac_selmode = SEL_none;
            SDLsac_selection[0] = -1;
            SDLsac_selection[1] = -1;
            SDLsac_selection[2] = -1;
            SDLsac_selection[3] = -1;

            SDL_SemPost( SDLsac_selectsem);
          }
          break;

        case SDL_MOUSEMOTION:
          if (SDLsac_selmode == SEL_bottom) {
            InvertRect( (Display) screen, SDLsac_selection);

            SDLsac_selection[2] = event.motion.x;
            SDLsac_selection[3] = event.motion.y;

            InvertRect( (Display) screen, SDLsac_selection);
          }
          break;

        default:
          break;
      }
    }
  }

  return 0;
}

static
Uint32 TimerHandler(Uint32 interval, void *param) {
  SDL_Event event;
#ifdef ADAPTIVE_MODE
  int eventqueue;
#endif

#ifdef ADAPTIVE_MODE
#ifdef UPDATE_VIA_SEMAPHORE
  eventqueue = SDL_SemValue( SDLsac_updatesem);
#else /* UPDATE_VIA_SEMAPHORE */
  eventqueue = SDL_PeepEvents( &event, 
                               1, 
                               SDL_PEEKEVENT, 
                               SDL_EVENTMASK( SDL_USEREVENT_DRAW));
#endif /* UPDATE_VIA_SEMAPHORE */

  if (eventqueue == 1) {
    /* an event overtook us, so we are too fast */
    interval *= 2;
  } else if (interval > MIN_UPDATE_INTERVAL + 10) {
    /* nothing waiting, we can speed up :-) */
    interval -= 10;
  }
#endif

  event.type = SDL_USEREVENT_DRAW;
  event.user.code = 0;
  event.user.data1 = param;
  event.user.data2 = NULL;

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

  disp = SDL_SetVideoMode( y, x, 32, SDL_SWSURFACE);

  if (disp == NULL) {
    SAC_RuntimeError( "Failed to init SDL Display: %s", SDL_GetError());
  }

  SDL_WM_SetCaption( SDL_SAC_DEFAULT_HEADING, NULL);
  SDL_ShowCursor( SDL_DISABLE);

  SDLsac_isasync = SAC_ND_A_FIELD( async_nt);

  /*
   * a shiny mutex 
   */
  SDLsac_mutex = SDL_CreateMutex();

  /*
   * semaphore for selection mode
   */
  SDLsac_selectsem = SDL_CreateSemaphore( 0);
  if (SDLsac_selectsem == NULL) {
    SAC_RuntimeError( "Failed to init selection semaphore");
  }

  if( SDLsac_isasync) {
#ifdef UPDATE_VIA_SEMAPHORE
    /*
     * create a semaphore
     */
    SDLsac_updatesem = SDL_CreateSemaphore( 0);
    if (SDLsac_updatesem == NULL) {
      SAC_RuntimeError( "Failed to init update semaphore");
    }

    /*
     * create the update thread
     */
    SDLsac_updater = SDL_CreateThread( UpdateScreen, disp);
#endif /* UPDATE_VIA_SEMAPHORE */
  
    /*
     * register an event handler 
     */ 
    SDLsac_eventhandler = SDL_CreateThread( EventHandler, NULL);
  
    /*
     * start a display update timer to update 20 times a second
     */
    SDLsac_timer = SDL_AddTimer( START_UPDATE_INTERVAL, TimerHandler, disp);
    if ( SDLsac_timer == NULL) {
      SAC_RuntimeError( "Failed to init update timer");
    }
  } 

  return( disp);
}
