#ifndef _STREAM_H_
#define _STREAM_H_

#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 32
#endif

#include "bool.h"
#include "task.h"
#include "spinlock.h"
#include "streamtab.h"
#include "atomic.h"


/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

typedef struct stream stream_t;

/* stream open descriptor */
struct stream_odesc {
  task_t *task;
  streamtbe_t *tbe;
  spinlock_t lock;
};

/* Padding is required to avoid false-sharing between core's private cache */
struct stream {
  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  void *buf[STREAM_BUFFER_SIZE];
  /* producer / consumer  */
  struct stream_odesc prod, cons;
  int wany_idx;
  atomic_t refcnt;
};


extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern bool StreamOpen(task_t *ct, stream_t *s, char mode);
extern void StreamClose(task_t *ct, stream_t *s);
extern void StreamReplace(task_t *ct, stream_t **s, stream_t *snew);
extern void *StreamPeek(task_t *ct, stream_t *s);
extern void *StreamRead(task_t *ct, stream_t *s);
extern bool StreamIsSpace(task_t *ct, stream_t *s);
extern void StreamWrite(task_t *ct, stream_t *s, void *item);

extern void StreamWaitAny(task_t *ct);
extern stream_t *StreamIterNext(task_t *ct);
extern bool StreamIterHasNext(task_t *ct);


#endif /* _STREAM_H_ */
