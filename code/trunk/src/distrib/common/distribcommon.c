#include <pthread.h>

#include "distribcommon.h"
#include "imanager.h"
#include "info.h"
#include "memfun.h"
#include "omanager.h"
#include "reference.h"
#include "snetentities.h"

static bool running = true;
static snet_info_tag_t prevDest;
static snet_info_tag_t infoCounter;
static pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exitMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  int *counter = SNetMemAlloc(sizeof(int));
  snet_dest_t *dest = SNetMemAlloc(sizeof(snet_dest_t));;

  *counter = 0;

  dest->node = 0;
  dest->dest = *counter;
  dest->parent = 0;
  dest->parentNode = 0;
  dest->dynamicIndex = 0;
  dest->dynamicLoc = 0;

  prevDest = SNetInfoCreateTag();
  SNetInfoSetTag(info, prevDest, (uintptr_t) dest,
                 (void* (*)(void*)) &SNetDestCopy);

  infoCounter = SNetInfoCreateTag();
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);

  SNetDistribImplementationInit(argc, argv, info);

  SNetReferenceInit();
  SNetOutputManagerInit();
  SNetInputManagerInit();
}

void SNetDistribStart(void)
{
  SNetOutputManagerStart();
  SNetInputManagerStart();
}

void SNetDistribStop(void)
{
  SNetDistribLocalStop();

  pthread_mutex_lock(&exitMutex);
  running = false;
  pthread_cond_signal(&exitCond);
  pthread_mutex_unlock(&exitMutex);
}

void SNetDistribWaitExit(snet_info_t *info)
{
  pthread_mutex_lock(&exitMutex);
  while (running) pthread_cond_wait(&exitCond, &exitMutex);
  pthread_mutex_unlock(&exitMutex);

  SNetMemFree((int*) SNetInfoDelTag(info, infoCounter));
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc)
{
  int *counter = (int*) SNetInfoGetTag(info, infoCounter);
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);

  if (dest->node != loc) {
    dest->dest = (*counter)++;

    if (SNetDistribIsNodeLocation(dest->node)) {
      dest->node = loc;
      SNetOutputManagerNewOut(*dest, in);

      in = NULL;
    } else if (SNetDistribIsNodeLocation(loc)) {
      if (in == NULL) in = SNetStreamCreate(0);

      SNetInputManagerNewIn(*dest, in);
      dest->node = loc;
    } else {
      dest->node = loc;
    }
  }

  return in;
}

void SNetRouteNewDynamic(snet_dest_t dest)
{
  snet_startup_fun_t fun = SNetIdToNet(dest.parent);

  snet_info_t *info = SNetInfoInit();
  SNetInfoSetTag(info, prevDest, (uintptr_t) SNetDestCopy(&dest),
                (void* (*)(void*)) &SNetDestCopy);

  SNetLocvecSet(info, SNetLocvecCreate());

  SNetRouteDynamicEnter(info, dest.dynamicIndex, dest.dynamicLoc, NULL);
  SNetRouteUpdate(info, fun(NULL, info, dest.dynamicLoc), dest.parentNode);
  SNetRouteDynamicExit(info, dest.dynamicIndex, dest.dynamicLoc, NULL);

  SNetInfoDestroy(info);
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  dest->dynamicIndex = dynamicIndex;

  int *counter = SNetMemAlloc(sizeof(int));
  *counter = 0;
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);

  if (fun != NULL) {
    dest->parent = SNetNetToId(fun);
    dest->parentNode = SNetDistribGetNodeId();
    dest->dynamicLoc = dynamicLoc;
  }

  return;
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  int *counter = (int*) SNetInfoDelTag(info, infoCounter);
  (void) dynamicIndex; /* NOT USED */
  (void) dynamicLoc; /* NOT USED */
  (void) fun; /* NOT USED */

  SNetMemFree(counter);
  SNetInfoSetTag(info, infoCounter, (uintptr_t) NULL, NULL);
  return;
}
