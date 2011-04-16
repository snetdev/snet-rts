#ifndef _STREAM_H_
#define _STREAM_H_

#include <pthread.h>

#include "threading.h"


#define SNET_STREAM_DEFAULT_CAPACITY  10


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
};


struct snet_stream_desc_t {
  snet_entity_t *entity;
  snet_stream_t *stream;
  char mode;
  struct snet_stream_desc_t *next;
};


#endif /* _STREAM_H_ */
