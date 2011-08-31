#include <pthread.h>

#include "distribution.h"
#include "iomanagers.h"

bool debugWait = false;

static int node_location;
static bool running = true;
static pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exitMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
    node_location = 0;
    SNetDataStorageInit();
}

void SNetDistribStart(void)
{
}

void SNetDistribStop(bool global)
{
  pthread_mutex_lock(&exitMutex);
  running = false;
  pthread_cond_signal(&exitCond);
  pthread_mutex_unlock(&exitMutex);
}

void SNetDistribWaitExit(void)
{
  pthread_mutex_lock(&exitMutex);
  while (running) pthread_cond_wait(&exitCond, &exitMutex);
  pthread_mutex_unlock(&exitMutex);
  SNetDataStorageDestroy();
}

int SNetDistribGetNodeId(void)
{
  return node_location;
}

bool SNetDistribIsNodeLocation(int location)
{
  /* with nodist, all entities should be built */
  return true;
}

bool SNetDistribIsRootNode(void)
{
  return true;
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
    return input;
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  return;
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  return;
}

void SNetDistribRemoteFetch(snet_ref_t *ref)
{
}

void SNetDistribRemoteUpdate(snet_ref_t *ref, int count)
{
}
