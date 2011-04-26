#ifndef _ENTITY_H_
#define _ENTITY_H_

#include <pthread.h>

#include "threading.h"

struct snet_entity_t {
  snet_entityfunc_t   func;
  void               *inarg;
  //pthread_t           thread;
  pthread_mutex_t     lock;
  pthread_cond_t      pollcond;
  snet_stream_desc_t *wakeup_sd;
};

snet_entity_t *SNetEntitySelf(void);

#endif /* _ENTITY_H_ */
