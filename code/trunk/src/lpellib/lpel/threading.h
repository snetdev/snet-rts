
#ifndef _THREADING_H_
#define _THREADING_H_

#define LPEL_USE_CAPABILITIES


#include <pthread.h>

#include "lpel.h"
#include "bool.h"




struct lpel_thread_t {
  pthread_t pthread;
  bool detached;
  void (*func)(void *);
  void *arg;
};


int _LpelThreadAssign( int core);

extern lpel_config_t    _lpel_global_config;

#endif /* _THREADING_H_ */
