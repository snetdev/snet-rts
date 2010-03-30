#include "SDLsac.h"
#include <stdlib.h>
#include <unistd.h>

int destroyDisplay( SAC_ND_PARAM_in_nodesc( disp_nt, Display))
{
  SDL_Event event;

  if (SDLsac_isasync) {
    /* stop the timer */
    if (SDLsac_timer != NULL) {
      SDL_RemoveTimer( SDLsac_timer);
    }

    /* tell the event handler to quit */
    event.type = SDL_USEREVENT_QUIT;
    event.user.code = 0;
    event.user.data1 = NULL;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    /* wait for event handler to finish */
    if (SDLsac_eventhandler != NULL) {
      SDL_WaitThread( SDLsac_eventhandler, NULL);
    }

#ifdef UPDATE_VIA_SEMAPHORE
    /* kill the updater */
    SDL_KillThread( SDLsac_updater);

    /* destroy the semaphore */
    SDL_DestroySemaphore( SDLsac_updatesem);
#endif
  }

  /* destroy the semaphore */
  SDL_DestroySemaphore( SDLsac_selectsem);

  /* finally, we can release this */
  if (SDLsac_mutex != NULL) {
    SDL_DestroyMutex( SDLsac_mutex);
  }

  SDL_Quit();
  return(0);
}
