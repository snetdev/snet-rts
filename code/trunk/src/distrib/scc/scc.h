#ifndef SCC_H
#define SCC_H

#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "SCC_API.h"

#define CORES                   (NUM_ROWS * NUM_COLS * NUM_CORES)
#define PAGE_SIZE               (16*1024*1024)
#define LINUX_PRIV_PAGES        (20)
#define PAGES_PER_CORE          (41)
#define MAX_PAGES               (172)
#define IRQ_BIT                 (0x01 << GLCFG_XINTR_BIT)

#define B_OFFSET                64
#define FOOL_WRITE_COMBINE      (mpbs[node_location][0] = 1)
#define START(mpb)              (*((volatile uint16_t *) ((mpb) + B_OFFSET)))
#define END(mpb)                (*((volatile uint16_t *) ((mpb) + B_OFFSET + 2)))
#define HANDLING(mpb)           (*(mpb + B_OFFSET + 4))
#define WRITING(mpb)            (*(mpb + B_OFFSET + 5))
#define B_START                 (B_OFFSET + 32)
#define B_SIZE                  (MPBSIZE - B_START)

#define LUT(loc, idx)           (*((volatile uint32_t*)(&luts[loc][idx])))

extern bool remap;
extern unsigned char node_location;

extern t_vcharp mpbs[CORES];
extern t_vcharp locks[CORES];
extern volatile int *irq_pins[CORES];
extern volatile uint64_t *luts[CORES];

/* Flush MPBT from L1. */
static inline void flush() { __asm__ volatile ( ".byte 0x0f; .byte 0x0a;\n" ); }

static inline void lock(unsigned char core) { while (!(*locks[core] & 0x01)); }

static inline void unlock(unsigned char core) { *locks[core] = 0; }

static inline void interrupt(unsigned char core)
{
  /* Possibly (unnecesarily) reads memory twice, which would cause slow down.
    * Needs testing to see if another implementation using a local variable
    * and only reading the remote CRB once is faster. */
  *irq_pins[core] |= IRQ_BIT;
  *irq_pins[core] &= ~IRQ_BIT;
}

static inline int min(int x, int y) { return x < y ? x : y; }

static inline void start_write_node(unsigned int node)
{ lock(node); }

static inline void stop_write_node(unsigned int node)
{
  int handling;
  flush();
  handling = HANDLING(mpbs[node]);

  if (!handling) interrupt(node);

  unlock(node);
}

static inline void cpy_mpb_to_mem(t_vcharp mpb, void *dst, int size)
{
  int start, end, cpy;

  flush();
  start = START(mpb);
  end = END(mpb);

  while (size > 0) {
    if (end < start) cpy = min(size, B_SIZE - start);
    else cpy = size;

    flush();
    memcpy(dst, (void*) (mpb + B_START + start), cpy);
    start = (start + cpy) % B_SIZE;
    dst = ((char*) dst) + cpy;
    size -= cpy;
  }

  flush();
  START(mpb) = start + size;
  FOOL_WRITE_COMBINE;
}

static inline void cpy_mem_to_mpb(t_vcharp mpb, void *src, int size)
{
  int start, end, oldEnd, cpy = 0;

  flush();
  start = START(mpb);
  oldEnd = end = END(mpb);

  while (size > 0) {
    while (cpy == 0) {
      if (end >= start) cpy = min(size, B_SIZE - end);
      else cpy = min(size, (start - 1) - end);

      if (cpy) break;

      unlock(node_location);
      usleep(10);
      lock(node_location);
      flush();
      start = START(mpb);

      if (start == oldEnd) SNetUtilDebugFatal("Message size to big!");
    }

    flush();
    memcpy((void*) (mpb + B_START + end), src, cpy);
    FOOL_WRITE_COMBINE;
    end = (end + cpy) % B_SIZE;
    src = ((char*) src) + cpy;
    size -= cpy;
  }

  flush();
  END(mpb) = end + size;
  FOOL_WRITE_COMBINE;
}
#endif
