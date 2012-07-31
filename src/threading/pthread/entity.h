#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "threading.h"


typedef struct {
  void               *inarg;
  pthread_mutex_t     lock;
  pthread_cond_t      pollcond;
  snet_stream_desc_t *wakeup_sd;
  char *name;
  bool run;
  snet_taskfun_t fun;
} snet_thread_t;

snet_thread_t *SNetThreadingSelf(void);

#endif /* _ENTITY_H_ */
