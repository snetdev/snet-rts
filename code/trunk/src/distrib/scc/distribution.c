#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "SCC_API.h"
#include "distribution.h"
#include "distribcommon.h"
#include "scc.h"
#include "sccmalloc.h"

unsigned char node_location;
t_vcharp mpbs[CORES];
t_vcharp locks[CORES];
volatile int *irq_pins[CORES];
volatile uint64_t *luts[CORES];

bool remap = false;
static int num_nodes = 0;

void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info)
{
  unsigned char num_pages;
  sigset_t signal_mask;
  int x, y, z;
  (void) info; /* NOT USED */

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-np") == 0 && ++i < argc) {
      num_nodes = atoi(argv[i]);
    } else if (strcmp(argv[i], "-sccremap") == 0) {
      remap = true;
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
  z = ReadConfigReg(CRB_OWN+MYTILEID);
  x = (z >> 3) & 0x0f; // bits 06:03
  y = (z >> 7) & 0x0f; // bits 10:07
  z = z & 7; // bits 02:00
  node_location = PID(x, y, z);

  for (unsigned char cpu = 0; cpu < CORES; cpu++) {
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

  num_pages = PAGES_PER_CORE - LINUX_PRIV_PAGES;
  int max_pages = remap ? MAX_PAGES/2 : MAX_PAGES - 1;

  for (int i = 1; i < CORES / num_nodes && num_pages < max_pages; i++) {
    for (int lut = 0; lut < PAGES_PER_CORE && num_pages < max_pages; lut++) {
      LUT(node_location, LINUX_PRIV_PAGES + num_pages++) = LUT(node_location + i * num_nodes, lut);
    }
  }

  int extra = ((CORES % num_nodes) * PAGES_PER_CORE) / num_nodes;
  int node = num_nodes + (node_location * extra) / PAGES_PER_CORE;
  int lut = (node_location * extra) % PAGES_PER_CORE;

  for (int i = 0; i < extra && num_pages < max_pages; i++ ) {
    LUT(node_location, LINUX_PRIV_PAGES + num_pages++) = LUT(node, lut + i);

    if (lut + i + 1 == PAGES_PER_CORE) {
      lut = 0;
      node++;
    }
  }

  flush();
  START(mpbs[node_location]) = 0;
  END(mpbs[node_location]) = 0;
  /* Start with an initial handling run to avoid a cross-core race. */
  HANDLING(mpbs[node_location]) = 1;

  SCCInit(num_pages);

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
  for (unsigned char cpu = 0; cpu < CORES; cpu++) {
    FreeConfigReg((int*) irq_pins[cpu]);
    FreeConfigReg((int*) locks[cpu]);
    FreeConfigReg((int*) luts[cpu]);
    MPBunalloc(&mpbs[cpu]);
  }

  SCCStop();
}

int SNetDistribGetNodeId(void) { return node_location; }

bool SNetDistribIsNodeLocation(int loc) { return node_location == loc; }

bool SNetDistribIsRootNode(void) { return node_location == 0; }

void SNetDistribPack(void *src, ...)
{
  bool isData;
  va_list args;
  lut_addr_t *addr;
  unsigned char node;
  size_t size, cpySize;

  va_start(args, src);
  addr = va_arg(args, void*);
  size = va_arg(args, size_t);
  isData = va_arg(args, bool);
  va_end(args);

  if (isData) {
    if (remap) {
      node = addr->node;
      *addr = SCCPtr2Addr(src);
      cpy_mem_to_mpb(mpbs[node], addr, sizeof(lut_addr_t));
      cpy_mem_to_mpb(mpbs[node], &size, sizeof(size_t));
    } else {
      while (size > 0) {
        flush();
        LUT(node_location, REMOTE_LUT) = LUT(addr->node, addr->lut++);

        cpySize = min(size, PAGE_SIZE - addr->offset);
        memcpy(((char*) remote + addr->offset), src, cpySize);

        size -= cpySize;
        src += cpySize;

        if (addr->offset) addr->offset = 0;
      }

      FOOL_WRITE_COMBINE;
    }
  } else {
    cpy_mem_to_mpb(mpbs[addr->node], src, size);
  }
}

void SNetDistribUnpack(void *dst, ...)
{
  bool isData;
  size_t size;
  va_list args;
  lut_addr_t *addr;

  va_start(args, dst);
  addr = va_arg(args, void*);
  isData = va_arg(args, bool);
  if (!isData) size = va_arg(args, size_t);
  va_end(args);

  if (isData) {
    if (remap) {
      unsigned char node, lut, count;
      cpy_mpb_to_mem(mpbs[node_location], addr, sizeof(lut_addr_t));
      cpy_mpb_to_mem(mpbs[node_location], &size, sizeof(size_t));

      node = addr->node;
      count = (size + addr->offset + PAGE_SIZE - 1) / PAGE_SIZE;
      lut = SCCMallocLut(count);

      for (unsigned char i = 0; i < count; i++) {
        LUT(node_location, lut + i) = LUT(node, addr->lut + i);
      }

      addr->lut = lut;
    }

    *(void**) dst = SCCAddr2Ptr(*addr);
  } else {
    cpy_mpb_to_mem(mpbs[node_location], dst, size);
  }
}
