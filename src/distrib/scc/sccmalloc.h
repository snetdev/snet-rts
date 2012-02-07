#ifndef SCCMALLOC_H
#define SCCMALLOC_H

#define LOCAL_LUT   0x14
#define REMOTE_LUT  (LOCAL_LUT + local_pages)

extern void *remote;
extern unsigned char local_pages;

typedef struct {
  unsigned char node, lut;
  uint32_t offset;
} lut_addr_t;

lut_addr_t SCCPtr2Addr(void *p);
void *SCCAddr2Ptr(lut_addr_t addr);

void SCCInit(unsigned char size);
void SCCStop(void);

void *SCCMallocPtr(size_t size);
unsigned char SCCMallocLut(size_t size);
void SCCFree(void *p);
#endif
