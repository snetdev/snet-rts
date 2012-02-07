#include <pthread.h>

#include "distribution.h"
#include "reference.h"

static int node_location;
static bool running = true;
static pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exitMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  (void) argc; /* NOT USED */
  (void) argv; /* NOT USED */
  (void) info; /* NOT USED */
  node_location = 0;
  SNetReferenceInit();
}

void SNetDistribStart(void) { }

void SNetDistribGlobalStop(void)
{
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
  SNetReferenceDestroy();
}

int SNetDistribGetNodeId(void) { return node_location; }

bool SNetDistribIsNodeLocation(int location)
{
  (void) location; /* NOT USED */
  /* with nodist, all entities should be built */
  return true;
}

bool SNetDistribIsRootNode(void) { return true; }

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
  (void) info; /* NOT USED */
  (void) input; /* NOT USED */
  (void) loc; /* NOT USED */

  return input;
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  (void) info; /* NOT USED */
  (void) dynamicIndex; /* NOT USED */
  (void) dynamicLoc; /* NOT USED */
  (void) fun; /* NOT USED */
  return;
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  (void) info; /* NOT USED */
  (void) dynamicIndex; /* NOT USED */
  (void) dynamicLoc; /* NOT USED */
  (void) fun; /* NOT USED */
  return;
}

void SNetDistribSendData(snet_ref_t *ref, void *data, int node) {}

void SNetDistribPack(void *src, ...) { (void) src; /* NOT USED */ }

void SNetDistribUnpack(void *dst, ...) { (void) dst; /* NOT USED */ }

void SNetDistribFetchRef(snet_ref_t *ref) { (void) ref; /* NOT USED */ }

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  (void) ref; /* NOT USED */
  (void) count; /* NOT USED */
}
