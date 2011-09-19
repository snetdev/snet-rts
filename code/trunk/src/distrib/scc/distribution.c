#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SCC_API.h"
#include "dest.h"
#include "distribution.h"
#include "imanager.h"
#include "iomanagers.h"
#include "memfun.h"
#include "omanager.h"
#include "reference.h"
#include "scc.h"
#include "snetentities.h"

int node_location;
t_vcharp mpbs[CORES];
t_vcharp locks[CORES];
volatile int *irq_pins[CORES];

static int num_nodes;
static bool running = true;
static snet_info_tag_t prevDest;
static snet_info_tag_t infoCounter;
static pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exitMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  snet_dest_t *dest;
  sigset_t signal_mask;
  int my_x, my_y, my_z, *counter;
  (void) info; /* NOT USED */

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-np") == 0 && ++i < argc) num_nodes = atoi(argv[i]);
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGUSR1);
  pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

  InitAPI(0);
  node_location = ReadConfigReg(CRB_OWN+MYTILEID);
  my_x = (node_location >> 3) & 0x0f; // bits 06:03
  my_y = (node_location >> 7) & 0x0f; // bits 10:07
  my_z = node_location & 7; // bits 02:00

  for (int cpu = 0; cpu < CORES; cpu++) {
    int address;
    int x = X_PID(cpu), y = Y_PID(cpu), z = Z_PID(cpu);

    if (x == my_x && y == my_y) {
      address = CRB_OWN;
      if (z == my_z) node_location = cpu;
    } else {
      address = CRB_ADDR(x, y);
    }

    irq_pins[cpu] = MallocConfigReg(address + (z ? GLCFG1 : GLCFG0));
    locks[cpu] = (t_vcharp) MallocConfigReg(address + (z ? LOCK1 : LOCK0));
    MPBalloc(&mpbs[cpu], x, y, z, x == my_x && y == my_y && z == my_z);
  }

  flush();
  START(mpbs[node_location]) = 0;
  END(mpbs[node_location]) = 0;
  /* Start with an initial handling run to avoid a cross-core race. */
  HANDLING(mpbs[node_location]) = 1;
  WRITING(mpbs[node_location]) = 0;
  FOOL_WRITE_COMBINE;
  unlock(node_location);

  counter = SNetMemAlloc(sizeof(int));
  *counter = 0;

  dest = SNetMemAlloc(sizeof(snet_dest_t));
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

  SNetReferenceInit();
  SNetOutputManagerInit();
  SNetInputManagerInit();
}

void SNetDistribStart(void)
{
  SNetOutputManagerStart();
  SNetInputManagerStart();
}

void SNetDistribStop(bool global)
{
  if (global) {
    int exit_status = 1;

    for (num_nodes = num_nodes - 1; num_nodes >= 0; num_nodes--) {
      start_write_node(num_nodes);
      cpy_mem_to_mpb(mpbs[num_nodes], &exit_status, sizeof(int));
      stop_write_node(num_nodes);
    }
  } else {
    pthread_mutex_lock(&exitMutex);
    running = false;
    pthread_cond_signal(&exitCond);
    pthread_mutex_unlock(&exitMutex);

    for (int cpu = 0; cpu < CORES; cpu++) {
      FreeConfigReg((int*)irq_pins[cpu]);
      FreeConfigReg((int *) locks[cpu]);
      MPBunalloc(&mpbs[cpu]);
    }
  }
}

void SNetDistribWaitExit(void)
{
  pthread_mutex_lock(&exitMutex);
  while (running) pthread_cond_wait(&exitCond, &exitMutex);
  pthread_mutex_unlock(&exitMutex);
}

int SNetDistribGetNodeId(void)
{
  return node_location;
}

bool SNetDistribIsNodeLocation(int location)
{
  return node_location == location;
}

bool SNetDistribIsRootNode(void)
{
  return node_location == 0;
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
  int *counter = (int*) SNetInfoGetTag(info, infoCounter);
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);

  if (dest->node != loc) {
    dest->dest = (*counter)++;

    if (dest->node == node_location) {
      dest->node = loc;
      SNetOutputManagerNewOut(SNetDestCopy(dest), input);

      input = NULL;
    } else if (loc == node_location) {
      if (input == NULL) input = SNetStreamCreate(0);

      SNetInputManagerNewIn(SNetDestCopy(dest), input);
      dest->node = loc;
    } else {
      dest->node = loc;
    }
  }

  return input;
}

void SNetDistribNewDynamicCon(snet_dest_t *dest)
{
  snet_dest_t *dst = SNetDestCopy(dest);
  snet_startup_fun_t fun = SNetIdToNet(dest->parent);

  snet_info_t *info = SNetInfoInit();
  SNetInfoSetTag(info, prevDest, (uintptr_t) dst,
                (void* (*)(void*)) &SNetDestCopy);

  SNetLocvecSet(info, SNetLocvecCreate());

  SNetRouteDynamicEnter(info, dest->dynamicIndex, dest->dynamicLoc, NULL);
  SNetRouteUpdate(info, fun(NULL, info, dest->dynamicLoc), dest->parentNode);
  SNetRouteDynamicExit(info, dest->dynamicIndex, dest->dynamicLoc, NULL);

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
    dest->parentNode = node_location;
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

void SNetDistribPack(void *src, ...)
{
  //va_list args;
  assert(0);
}

void SNetDistribUnpack(void *dst, ...)
{
  //va_list args;
  assert(0);
}

void SNetDistribRemoteFetch(snet_ref_t *ref)
{
  assert(0);
}

void SNetDistribRemoteUpdate(snet_ref_t *ref, int count)
{
  assert(0);
}
