#ifndef SCC_H
#define SCC_H

#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "SCC_API.h"
#include "bool.h"

#define CORES               (NUM_ROWS * NUM_COLS * NUM_CORES)
#define PAGE_SIZE           (16*1024*1024)
#define LINUX_PRIV_PAGES    (20)
#define PAGES_PER_CORE      (41)
#define MAX_PAGES           (172)
#define IRQ_BIT             (0x01 << GLCFG_XINTR_BIT)

#define B_OFFSET            64
#define FOOL_WRITE_COMBINE  (mpbs[node_location][0] = 1)
#define START(i)            (*((volatile uint16_t *) (mpbs[i] + B_OFFSET)))
#define END(i)              (*((volatile uint16_t *) (mpbs[i] + B_OFFSET + 2)))
#define HANDLING(i)         (*(mpbs[i] + B_OFFSET + 4))
#define WRITING(i)          (*(mpbs[i] + B_OFFSET + 5))
#define B_START             (B_OFFSET + 32)
#define B_SIZE              (MPBSIZE - B_START)

#define LUT(loc, idx)       (*((volatile uint32_t*)(&luts[loc][idx])))

extern bool remap;
extern int node_location;

extern t_vcharp mpbs[CORES];
extern t_vcharp locks[CORES];
extern volatile int *irq_pins[CORES];
extern volatile uint64_t *luts[CORES];

static inline int min(int x, int y) { return x < y ? x : y; }

/* Flush MPBT from L1. */
static inline void flush() { __asm__ volatile ( ".byte 0x0f; .byte 0x0a;\n" ); }

static inline void lock(int core) { while (!(*locks[core] & 0x01)); }

static inline void unlock(int core) { *locks[core] = 0; }

static inline void interrupt(int core)
{
  /* Possibly (unnecesarily) reads memory twice, which would cause slow down.
    * Needs testing to see if another implementation using a local variable
    * and only reading the remote CRB once is faster. */
  *irq_pins[core] |= IRQ_BIT;
  *irq_pins[core] &= ~IRQ_BIT;
}

static inline void start_write_node(int node)
{
  while (true) {
    lock(node);
    flush();
    if (!WRITING(node)) return;
    unlock(node);
    usleep(1);
  }
}

static inline void stop_write_node(int node)
{
  flush();
  if (!HANDLING(node)) interrupt(node);
  unlock(node);
}

static inline void cpy_mpb_to_mem(int node, void *dst, int size)
{
  int start, end, cpy;

  flush();
  start = START(node);
  end = END(node);

  while (size) {
    if (end < start) cpy = min(size, B_SIZE - start);
    else cpy = size;

    flush();
    memcpy(dst, (void*) (mpbs[node] + B_START + start), cpy);
    start = (start + cpy) % B_SIZE;
    dst = ((char*) dst) + cpy;
    size -= cpy;
  }

  flush();
  START(node) = start;
  FOOL_WRITE_COMBINE;
}

static inline void cpy_mem_to_mpb(int node, void *src, int size)
{
  int start, end, free;

  if (size >= B_SIZE) SNetUtilDebugFatal("Message to big!");

  flush();
  WRITING(node) = true;
  FOOL_WRITE_COMBINE;

  while (size) {
    flush();
    start = START(node);
    end = END(node);

    if (end < start) free = start - end - 1;
    else free = B_SIZE - end - (start == 0 ? 1 : 0);
    free = min(free, size);

    if (!free) {
      unlock(node);
      usleep(1);
      lock(node);
      continue;
    }

    memcpy((void*) (mpbs[node] + B_START + end), src, free);

    flush();
    size -= free;
    src += free;
    END(node) = (end + free) % B_SIZE;
    WRITING(node) = false;
    FOOL_WRITE_COMBINE;
  }
}
#endif
