#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "SCC_API.h"
#include "dest.h"
#include "distribution.h"
#include "iomanagers.h"
#include "memfun.h"
#include "snetentities.h"

#define CORES               (NUM_ROWS * NUM_COLS * NUM_CORES)
#define IRQ_BIT             (0x01 << GLCFG_XINTR_BIT)

bool debugWait = false;

static int *irq_pins[CORES];
static int node_location;
static snet_info_tag_t prevDest;
static snet_info_tag_t infoCounter;
static bool running = true;
static pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exitMutex = PTHREAD_MUTEX_INITIALIZER;

static void write_pid(void)
{
    FILE *file = fopen("/sys/module/async_scc/parameters/pid", "w");
    if (file == NULL) {
        perror("Could not open module parameter");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%d", getpid());
    fclose(file);
}

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  snet_dest_t *dest;
  int cpu, my_x, my_y, my_z, *counter;
  (void) argc; /* NOT USED */
  (void) argv; /* NOT USED */
  (void) info; /* NOT USED */

  InitAPI(0);
  node_location = ReadConfigReg(CRB_OWN+MYTILEID);
  my_x = (node_location >> 3) & 0x0f; // bits 06:03
  my_y = (node_location >> 7) & 0x0f; // bits 10:07
  my_z = node_location & 7; // bits 02:00

  for (cpu = 0; cpu < CORES; cpu++) {
    int address;
    int x = X_PID(cpu);
    int y = Y_PID(cpu);
    int z = Z_PID(cpu);

    if (x == my_x && y == my_y) {
      address = CRB_OWN;
      if (z == my_z) node_location = cpu;
    } else {
      address = CRB_ADDR(x, y);
    }

    irq_pins[cpu] = MallocConfigReg(address + (z ? GLCFG1 : GLCFG0));
  }

  write_pid();

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

  SNetDataStorageInit();
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
    assert(0);
  } else {
    pthread_mutex_lock(&exitMutex);
    running = false;
    pthread_cond_signal(&exitCond);
    pthread_mutex_unlock(&exitMutex);
  }
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
      SNetDistribNewOut(SNetDestCopy(dest), input);

      input = NULL;
    } else if (loc == node_location) {
      if (input == NULL) input = SNetStreamCreate(0);

      SNetDistribNewIn(SNetDestCopy(dest), input);
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
