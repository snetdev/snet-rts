#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "SCC_API.h"
#include "distribution.h"
#include "distribcommon.h"
#include "scc.h"

#define SIZE_16MB   (16*1024*1024)

int node_location;
t_vcharp mpbs[CORES];
t_vcharp locks[CORES];
volatile int *irq_pins[CORES];
volatile uint64_t *luts[CORES];

static char *local, *remote;
static int mem, cache, num_nodes = 0, num_pages = 0;

void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info)
{
  sigset_t signal_mask;
  int x, y, z;
  (void) info; /* NOT USED */

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-np") == 0 && ++i < argc) {
      num_nodes = atoi(argv[i]);
      num_pages = min(171, 2048/num_nodes - 20);
    }
  }

  if (num_nodes == 0) {
    SNetUtilDebugFatal("Number of nodes not specified using -np flag!\n");
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGUSR1);
  sigaddset(&signal_mask, SIGUSR2);
  pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

  InitAPI(0);
  node_location = ReadConfigReg(CRB_OWN+MYTILEID);
  x = (node_location >> 3) & 0x0f; // bits 06:03
  y = (node_location >> 7) & 0x0f; // bits 10:07
  z = node_location & 7; // bits 02:00
  node_location = PID(x, y, z);

  for (int cpu = 0; cpu < num_nodes; cpu++) {
    int address;
    x = X_PID(cpu);
    y = Y_PID(cpu);
    z = Z_PID(cpu);

    if (cpu == node_location) address = CRB_OWN;
    else address = CRB_ADDR(x, y);

    irq_pins[cpu] = MallocConfigReg(address + (z ? GLCFG1 : GLCFG0));
    luts[cpu] = (uint64_t*) MallocConfigReg(address + (z ? LUT1 : LUT0));
    locks[cpu] = (t_vcharp) MallocConfigReg(address + (z ? LOCK1 : LOCK0));
    MPBalloc(&mpbs[cpu], x, y, z, cpu == node_location);
  }

  flush();
  START(mpbs[node_location]) = 0;
  END(mpbs[node_location]) = 0;
  /* Start with an initial handling run to avoid a cross-core race. */
  HANDLING(mpbs[node_location]) = 1;
  FOOL_WRITE_COMBINE;

  /* Open driver device "/dev/rckdyn110" to map memory in write-through mode */
  mem = open("/dev/rckdyn110", O_RDWR|O_SYNC);
  if (mem < 0) SNetUtilDebugFatal("Opening /dev/rckdyn110 failed!");
  cache = open("/dev/rckdcm", O_RDWR|O_SYNC);
  if (cache < 0) SNetUtilDebugFatal("Opening /dev/rckdcm failed!");

  remote = mmap(NULL, SIZE_16MB, PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0x14 << 24);
  if (remote == NULL) SNetUtilDebugFatal("Couldn't map memory!");

  local = mmap(NULL, num_pages * SIZE_16MB, PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0x15 << 24);
  if (local == NULL) SNetUtilDebugFatal("Couldn't map memory!");

  FOOL_WRITE_COMBINE;
  unlock(node_location);
}

void SNetDistribGlobalStop(void)
{
  snet_comm_type_t exit_status = snet_stop;

  for (int i = num_nodes - 1; i >= 0; i--) {
    start_write_node(i);
    cpy_mem_to_mpb(mpbs[i], &exit_status, sizeof(snet_comm_type_t));
    stop_write_node(i);
  }
}

void SNetDistribLocalStop(void)
{
  for (int cpu = 0; cpu < num_nodes; cpu++) {
    FreeConfigReg((int*) irq_pins[cpu]);
    FreeConfigReg((int*) locks[cpu]);
    FreeConfigReg((int*) luts[cpu]);
    MPBunalloc(&mpbs[cpu]);
  }

  munmap(remote, SIZE_16MB);
  munmap(local, num_pages * SIZE_16MB);

  close(mem);
  close(cache);
}

int SNetDistribGetNodeId(void) { return node_location; }

bool SNetDistribIsNodeLocation(int loc) { return node_location == loc; }

bool SNetDistribIsRootNode(void) { return node_location == 0; }

void SNetDistribPack(void *src, ...)
{
  va_list args;
  t_vcharp buf;
  size_t size;

  va_start(args, src);
  buf = va_arg(args, t_vcharp);
  size = va_arg(args, size_t);
  va_end(args);

  flush();
  cpy_mem_to_mpb(buf, src, size);
  FOOL_WRITE_COMBINE;
}

void SNetDistribUnpack(void *dst, ...)
{
  va_list args;
  t_vcharp buf;
  size_t size;

  va_start(args, dst);
  buf = va_arg(args, void*);
  size = va_arg(args, size_t);
  va_end(args);

  flush();
  cpy_mpb_to_mem(buf, dst, size);
  FOOL_WRITE_COMBINE;
}
