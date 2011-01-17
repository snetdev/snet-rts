#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <pcl.h>

#include "lpel.h"

#include "arch/timing.h"
#include "bool.h"
#include "scheduler.h"
#include "monitoring.h"
//#include "mailbox.h"
#include "mailbox-lf.h"
#include "taskqueue.h"



typedef struct {
  int node;
  bool do_print_workerinfo;
} workercfg_t;



typedef struct {
  int wid; 
  pthread_t     thread;
  coroutine_t   mctx;
  unsigned int  num_tasks;
  unsigned int  loop;
  bool          terminate;
  timing_t      wait_time;
  unsigned int  wait_cnt;
  int           req_pending;
  taskqueue_t   free_tasks;
  mailbox_t     mailbox;
  schedctx_t   *sched;
  monitoring_t *mon;
} workerctx_t;





void _LpelWorkerInit( int size, workercfg_t *cfg);
void _LpelWorkerCleanup( void);

void _LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);



#endif /* _WORKER_H_ */
