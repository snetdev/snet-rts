#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>

#include "lpel.h"

#include "bool.h"
#include "scheduler.h"
#include "monitoring.h"
#include "mailbox.h"




typedef struct {
  int node;
  bool do_print_workerinfo;
} workercfg_t;



typedef struct {
  int wid; 
  pthread_t     thread;
  unsigned int  num_tasks;
  unsigned int  loop;
  bool          terminate;  
  mailbox_t     mailbox;
  schedctx_t   *sched;
  monitoring_t *mon;
} workerctx_t;





void _LpelWorkerInit( int size, workercfg_t *cfg);
void _LpelWorkerCleanup( void);

void _LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);



#endif /* _WORKER_H_ */
