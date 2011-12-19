#ifndef SCCMALLOC_H
#define SCCMALLOC_H

typedef struct {
  unsigned char node, lut;
  uint32_t offset;
} lut_addr_t;

lut_addr_t SCCPtr2Addr(void *);
void *SCCAddr2Ptr(lut_addr_t);

void SCCInit(void *buffer, unsigned char start, size_t size);
void *SCCMalloc(size_t size);
void SCCFree(void *p);
#endif
