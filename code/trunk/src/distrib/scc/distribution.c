#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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

#define SIZE_16MB   (16*1024*1024)

char *localSpace;
int node_location;
t_vcharp mpbs[CORES];
t_vcharp locks[CORES];
volatile int *irq_pins[CORES];
volatile uint64_t *luts[CORES];

static int fd;

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

  for (int cpu = 0; cpu < num_nodes; cpu++) {
    int address;
    int x = X_PID(cpu), y = Y_PID(cpu), z = Z_PID(cpu);

    if (x == my_x && y == my_y) {
      address = CRB_OWN;
      if (z == my_z) node_location = cpu;
    } else {
      address = CRB_ADDR(x, y);
    }

    irq_pins[cpu] = MallocConfigReg(address + (z ? GLCFG1 : GLCFG0));
    luts[cpu] = (uint64_t*) MallocConfigReg(address + (z ? LUT1 : LUT0));
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

  /* Open driver device "/dev/rckdyn110" to map memory in write-through mode */
  fd = open("/dev/rckdyn110", O_RDWR|O_SYNC);
  if (fd < 0) SNetUtilDebugFatal("Opening /dev/rckdyn110 failed!");

  localSpace = mmap(NULL, SIZE_16MB, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x16 << 24);
  if (localSpace == NULL) SNetUtilDebugFatal("Couldn't map memory!");

  *((volatile uint32_t*) localSpace) = 0;
  *((volatile uint32_t*) (localSpace + 4)) = 0;
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

void SNetDistribStop(bool global)
{
  if (global) {
    snet_comm_type_t exit_status = snet_stop;

    for (int i = num_nodes - 1; i >= 0; i--) {
      start_write_node(i);
      cpy_mem_to_mpb(mpbs[i], &exit_status, sizeof(snet_comm_type_t));
      stop_write_node(i);
    }
  } else {
    pthread_mutex_lock(&exitMutex);
    running = false;
    pthread_cond_signal(&exitCond);
    pthread_mutex_unlock(&exitMutex);

    for (int cpu = 0; cpu < num_nodes; cpu++) {
      FreeConfigReg((int*) irq_pins[cpu]);
      FreeConfigReg((int*) locks[cpu]);
      FreeConfigReg((int*) luts[cpu]);
      MPBunalloc(&mpbs[cpu]);
    }
    munmap(localSpace, min(106, (32 * 1024) / (16 * num_nodes)) * SIZE_16MB);
  }
}

int SNetDistribGetNodeId(void) { return node_location; }

bool SNetDistribIsNodeLocation(int loc) { return node_location == loc; }

bool SNetDistribIsRootNode(void) { return node_location == 0; }

void SNetDistribPack(void *src, ...)
{
  char *data;
  va_list args;
  int dest, start, end;
  size_t size;

  va_start(args, src);
  dest = va_arg(args, int);
  size = va_arg(args, size_t);
  va_end(args);
  *((uint32_t*) &luts[node_location][0x15]) = *((uint32_t*) &luts[dest][0x16]);

  data = mmap(NULL, SIZE_16MB, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x15 << 24);

  flush();
  start = *((volatile uint32_t*) data);
  end = *((volatile uint32_t*) (data + 4));
  flush();
  memcpy(data + 8 + end, src, size);
  FOOL_WRITE_COMBINE;
  flush();
  *((volatile uint32_t*) (data + 4)) = end + size;
  FOOL_WRITE_COMBINE;
  munmap(data, SIZE_16MB);
}

void SNetDistribUnpack(void *dst, ...)
{
  va_list args;
  char *local;
  size_t size;
  int start, end;

  va_start(args, dst);
  local = va_arg(args, void*);
  size = va_arg(args, size_t);
  va_end(args);

  flush();
  start = *((volatile uint32_t*) local);
  end = *((volatile uint32_t*) (local + 4));
  memcpy(dst, local + 8 + start, size);
  flush();
  *((volatile uint32_t*) local) = start + size;
  FOOL_WRITE_COMBINE;
}
