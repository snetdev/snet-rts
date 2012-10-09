#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>

#include "memfun.h"
#include "scc.h"
#include "sccmalloc.h"
#include "bool.h"

typedef union block {
  struct {
    union block *next;
    size_t size;
  } hdr;
  uint32_t align;   // Forces proper allignment
} block_t;

typedef struct {
  unsigned char free;
  unsigned char size;
} lut_state_t;

void *remote;
unsigned char local_pages;

static void *local;
static int mem, cache;
static block_t *freeList;
static lut_state_t *lutState;
static unsigned char remote_pages;

lut_addr_t SCCPtr2Addr(void *p)
{
  uint32_t offset;
  unsigned char lut;

  if (local <= p && p <= local + local_pages * PAGE_SIZE) {
    offset = (p - local) % PAGE_SIZE;
    lut = LOCAL_LUT + (p - local) / PAGE_SIZE;
  } else if (remote <= p && p <= remote + remote_pages * PAGE_SIZE) {
    offset = (p - remote) % PAGE_SIZE;
    lut = REMOTE_LUT + (p - remote) / PAGE_SIZE;
  } else {
    SNetUtilDebugFatal("Invalid pointer\n");
  }

  lut_addr_t result = {node_location, lut, offset};
  return result;
}

void *SCCAddr2Ptr(lut_addr_t addr)
{
  if (LOCAL_LUT <= addr.lut && addr.lut < LOCAL_LUT + local_pages) {
    return (void*) ((addr.lut - LOCAL_LUT) * PAGE_SIZE + addr.offset + local);
  } else if (REMOTE_LUT <= addr.lut && addr.lut < REMOTE_LUT + remote_pages) {
    return (void*) ((addr.lut - REMOTE_LUT) * PAGE_SIZE + addr.offset + remote);
  } else {
    SNetUtilDebugFatal("Invalid SCC LUT address\n");
  }

  return NULL;
}

void SCCInit(unsigned char size)
{
  local_pages = size;
  remote_pages = remap ? MAX_PAGES - size : 1;

  /* Open driver device "/dev/rckdyn011" to map memory in write-through mode */
  mem = open("/dev/rckdyn011", O_RDWR|O_SYNC);
  if (mem < 0) SNetUtilDebugFatal("Opening /dev/rckdyn011 failed!");
  cache = open("/dev/rckdcm", O_RDWR|O_SYNC);
  if (cache < 0) SNetUtilDebugFatal("Opening /dev/rckdcm failed!");

  local = mmap(NULL, local_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem, LOCAL_LUT << 24);
  if (local == NULL) SNetUtilDebugFatal("Couldn't map memory!");

  remote = mmap(NULL, remote_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem, REMOTE_LUT << 24);
  if (remote == NULL) SNetUtilDebugFatal("Couldn't map memory!");

  freeList = local;
  freeList->hdr.next = freeList;
  freeList->hdr.size = (size * PAGE_SIZE) / sizeof(block_t);

  if (remap) {
    lutState = SNetMemAlloc(remote_pages * sizeof(lut_state_t));
    lutState[0].free = 1;
    lutState[0].size = remote_pages;
    lutState[remote_pages - 1] = lutState[0];
  }
}

void SCCStop(void)
{
  munmap(remote, remote_pages * PAGE_SIZE);
  munmap(local, local_pages * PAGE_SIZE);

  close(mem);
  close(cache);
}

void *SCCMallocPtr(size_t size)
{
  size_t nunits;
  block_t *curr, *prev, *new;

  if (freeList == NULL) SNetUtilDebugFatal("Couldn't allocate memory!");

  prev = freeList;
  curr = prev->hdr.next;
  nunits = (size + sizeof(block_t) - 1) / sizeof(block_t) + 1;

  do {
    if (curr->hdr.size >= nunits) {
      if (curr->hdr.size == nunits) {

        if (prev == curr) prev = NULL;
        else prev->hdr.next = curr->hdr.next;

      } else if (curr->hdr.size > nunits) {
        new = curr + nunits;
        *new = *curr;
        new->hdr.size -= nunits;
        curr->hdr.size = nunits;

        if (prev == curr) prev = new;
        prev->hdr.next = new;
      }

      freeList = prev;
      return (void*) (curr + 1);
    }
  } while (curr != freeList && (prev = curr, curr = curr->hdr.next));

  SNetUtilDebugFatal("Couldn't allocate memory!");
  return NULL;
}

void SCCFreePtr(void *p)
{
  block_t *block = (block_t*) p - 1,
          *curr = freeList;

  if (freeList == NULL) {
    freeList = block;
    freeList->hdr.next = freeList;
    return;
  }

  while (!(block > curr && block < curr->hdr.next)) {
    if (curr >= curr->hdr.next && (block > curr || block < curr->hdr.next)) break;
    curr = curr->hdr.next;
  }


  if (block + block->hdr.size == curr->hdr.next) {
    block->hdr.size += curr->hdr.next->hdr.size;
    if (curr == curr->hdr.next) block->hdr.next = block;
    else block->hdr.next = curr->hdr.next->hdr.next;
  } else {
    block->hdr.next = curr->hdr.next;
  }

  if (curr + curr->hdr.size == block) {
    curr->hdr.size += block->hdr.size;
    curr->hdr.next = block->hdr.next;
  } else {
    curr->hdr.next = block;
  }

  freeList = curr;
}

unsigned char SCCMallocLut(size_t size)
{
  lut_state_t *curr = lutState;

  do {
    if (curr->free && curr->size >= size) {
      if (curr->size == size) {
        curr->free = 0;
        curr[size - 1].free = 0;
      } else {
        lut_state_t *next = curr + size;

        next->free = 1;
        next->size = curr->size - size;
        next[next->size - 1] = next[0];

        curr->free = 0;
        curr->size = size;
        curr[curr->size - 1] = curr[0];
      }

      return REMOTE_LUT + curr - lutState;
    }

    curr += curr->size;
  } while (curr < lutState + remote_pages);

  SNetUtilDebugFatal("Not enough available LUT entries!\n");
  return 0;
}

void SCCFreeLut(void *p)
{
  lut_state_t *lut = lutState + (p - remote) / PAGE_SIZE;

  if (lut + lut->size < lutState + remote_pages && lut[lut->size].free) {
    lut->size += lut[lut->size].size;
  }

  if (lutState < lut && lut[-1].free) {
    lut -= lut[-1].size;
    lut->size += lut[lut->size].size;
  }

  lut->free = 1;
  lut[lut->size - 1] = lut[0];
}

void SCCFree(void *p)
{
  if (local <= p && p <= local + local_pages * PAGE_SIZE) {
    SCCFreePtr(p);
  } else if (remote <= p && p <= remote + remote_pages * PAGE_SIZE) {
    SCCFreeLut(p);
  }
}
