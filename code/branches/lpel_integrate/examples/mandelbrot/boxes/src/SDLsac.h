#ifndef _SDLSAC_H_
#define _SDLSAC_H_

#include "sac.h"
#include "SDL.h"
#include "SDL_thread.h"

#undef UPDATE_VIA_SEMAPHORE
#define ADAPTIVE_MODE

typedef SDL_Surface* Display;

extern SDL_Thread *SDLsac_eventhandler;
extern SDL_mutex *SDLsac_mutex;
extern SDL_TimerID SDLsac_timer;

#ifdef UPDATE_VIA_SEMAPHORE
extern SDL_sem *SDLsac_updatesem;
extern SDL_Thread *SDLsac_updater;
#endif /* UPDATE_VIA_SEMAPHORE */

extern bool SDLsac_isasync;

typedef enum {SEL_none, SEL_top, SEL_bottom} selmode_t;
extern selmode_t SDLsac_selmode;
extern SDL_sem *SDLsac_selectsem;
extern int SDLsac_selection[4];

#define SDL_SAC_DEFAULT_HEADING "SaC SDL Display"
#define SDL_SAC_SELECT_HEADING "Click and drag to select an area, button-two click to cancel..."

#define SDL_USEREVENT_DRAW (SDL_USEREVENT + 1)
#define SDL_USEREVENT_QUIT (SDL_USEREVENT + 2)

#define disp_nt       (disp, T_OLD((SCL, (HID, (NUQ,)))))
#define shp_nt        (shp, T_OLD((AKS, (NHD, (NUQ,)))))
#define ar_nt         (ar, T_OLD((AKD, (NHD, (NUQ,)))))
#define color_nt      (col, T_OLD((AKS, (NHD, (NUQ,)))))
#define async_nt      (async, T_OLD((SCL, (NHD, (NUQ,)))))
#define aks_out_nt    (aks_out, T_OLD((AKS, (NHD, (NUQ,)))))
#define aks_nt        (aks, T_OLD((AKS, (NHD, (NUQ,)))))

#endif
