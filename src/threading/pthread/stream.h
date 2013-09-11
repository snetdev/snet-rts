#ifndef _STREAM_H_
#define _STREAM_H_

#include <pthread.h>

#include "threading.h"
#include "entity.h"

/* Keep this in sync with include/stream.h. */
#define SNET_STREAM_DEFAULT_CAPACITY  16


struct snet_stream_t {
  pthread_mutex_t lock;
  pthread_cond_t  notempty;
  pthread_cond_t  notfull;

  void **buffer;
  int head, tail;
  int size;
  int count;

  snet_stream_desc_t *producer;
  snet_stream_desc_t *consumer;

  int is_poll;

  snet_locvec_t *source;

  struct {
    void (*func)(void*);
    void *arg;
  } callback_read;
};


struct snet_stream_desc_t {
  snet_thread_t *thr;
  snet_stream_t *stream;
  char mode;
  struct snet_stream_desc_t *next;
};


#endif /* _STREAM_H_ */
