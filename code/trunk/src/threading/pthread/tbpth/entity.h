#ifndef _ENTITY_H_
#define _ENTITY_H_

//#include <pthread.h>

#include "threading.h"


typedef struct {
  snet_entity_t      *entity;
  // void               *inarg;
  //pthread_t           thread;
  pthread_mutex_t     lock;
  pthread_cond_t      pollcond;
  snet_stream_desc_t *wakeup_sd;
} snet_thread_t;

extern snet_thread_t *SNetThreadingSelf(void);

#endif /* _ENTITY_H_ */
