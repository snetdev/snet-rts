#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "scc.h"
#include "sccmalloc.h"

typedef union block {
  struct {
    union block *next;
    size_t size;
  } hdr;
  uint32_t align;   // Forces proper allignment
} block_t;

static void *startPtr;
static unsigned char startLut;
static block_t *freeList = NULL;

lut_addr_t SCCPtr2Addr(void *ptr)
{
  uint32_t offset = (ptr - startPtr) % PAGE_SIZE;
  unsigned char lut = startLut + (ptr - startPtr) / PAGE_SIZE;
  lut_addr_t result = {node_location, lut, offset};
  return result;
}

void *SCCAddr2Ptr(lut_addr_t addr)
{ return (void*) ((addr.lut - startLut) * PAGE_SIZE + addr.offset + startPtr); }

void SCCInit(void *buffer, unsigned char start, size_t size)
{
  startPtr = buffer;
  startLut = start;
  freeList = buffer;
  freeList->hdr.next = freeList;
  freeList->hdr.size = size/sizeof(block_t);
}

void *SCCMalloc(size_t size)
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

void SCCFree(void *p)
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
