#ifndef _TASK_H_
#define _TASK_H_


#include <pthread.h>
#include <pcl.h>    /* tasks are executed in user-space with help of
                       GNU Portable Coroutine Library  */

#include "arch/timing.h"
#include "arch/atomic.h"

#include "lpel.h"
#include "worker.h"


/**
 * If a stacksize attribute <= 0 is specified,
 * use the default stacksize
 */
#define TASK_ATTR_STACKSIZE_DEFAULT  8192  /* 8k stacksize*/




typedef enum {
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_BLOCKED = 'B',
  TASK_ZOMBIE  = 'Z'
} taskstate_t;

typedef enum {
  WAIT_ON_READ  = 'r',
  WAIT_ON_WRITE = 'w',
  WAIT_ON_ANY   = 'a'
} taskstate_wait_t;



/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  lpel_task_t *prev, *next;
  unsigned long uid;        /** unique identifier */
  lpel_taskattr_t attr;     /** attributes */
  taskstate_t state;        /** state */
  taskstate_wait_t wait_on; /** on which event the task is waiting */

  workerctx_t *worker_context;  /** worker context for this task */

  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  lpel_stream_desc_t *wakeup_sd;
  atomic_t poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  /** timestamps for creation, start/stop of last dispatch */
  struct {
    timing_t creat, start, stop;
  } times;
  /** dispatch counter */
  unsigned long cnt_dispatch;
  /** streams marked as dirty */
  lpel_stream_desc_t *dirty_list;

  /* CODE */
  coroutine_t ctx;      /** context of the task*/
  lpel_taskfunc_t code; /** function of the task */
  void *inarg;          /** input argument  */
};




void _LpelTaskCall(    lpel_task_t *ct);
void _LpelTaskBlock(   lpel_task_t *ct, int wait_on);
void _LpelTaskDestroy( lpel_task_t *t);


#endif /* _TASK_H_ */
