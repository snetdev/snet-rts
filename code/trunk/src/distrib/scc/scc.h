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
{
  while (true) {
    lock(node);
    flush();
    if (!WRITING(mpbs[node])) return;
    unlock(node);
    usleep(1);
  }
}

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

  while (size) {
    if (end < start) cpy = min(size, B_SIZE - start);
    else cpy = size;

    flush();
    memcpy(dst, (void*) (mpb + B_START + start), cpy);
    start = (start + cpy) % B_SIZE;
    dst = ((char*) dst) + cpy;
    size -= cpy;
  }

  flush();
  START(mpb) = start;
  FOOL_WRITE_COMBINE;
}

static inline void cpy_mem_to_mpb(t_vcharp mpb, void *src, int size)
{
  int start, end, free;

  if (size >= B_SIZE) SNetUtilDebugFatal("Message to big!");

  flush();
  WRITING(mpb) = true;
  FOOL_WRITE_COMBINE;

  while (size) {
    flush();
    start = START(mpb);
    end = END(mpb);

    if (end < start) free = start - end - 1;
    else free = B_SIZE - end - (start == 0 ? 1 : 0);
    free = min(free, size);

    if (!free) {
      int i;
      for (i = 0; i < CORES; i++) if (mpbs[i] == mpb) break;

      unlock(i);
      usleep(1);
      lock(i);
      continue;
    }

    memcpy((void*) (mpb + B_START + end), src, free);

    flush();
    size -= free;
    src += free;
    END(mpb) = (end + free) % B_SIZE;
    WRITING(mpb) = false;
    FOOL_WRITE_COMBINE;
  }
}
#endif
