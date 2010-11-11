#ifndef _BUFFER_H_
#define _BUFFER_H_


#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 10
#endif


#include "sysdep.h"

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing
   between core's private cache */
typedef struct buffer {
  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  void *data[STREAM_BUFFER_SIZE];
} buffer_t;


void BufferReset( buffer_t *buf);
void *BufferTop( buffer_t *buf);
void BufferPop( buffer_t *buf);
int BufferIsSpace( buffer_t *buf);
void BufferPut( buffer_t *buf, void *item);

#endif /* _BUFFER_H_ */
