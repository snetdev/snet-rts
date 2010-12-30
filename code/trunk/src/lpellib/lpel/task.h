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
  TASK_CREATED = 'C',
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_BLOCKED = 'B',
  TASK_ZOMBIE  = 'Z'
} taskstate_t;

typedef enum {
  BLOCKED_ON_INPUT  = 'i',
  BLOCKED_ON_OUTPUT = 'o',
  BLOCKED_ON_ANYIN  = 'a'
} taskstate_blocked_t;


typedef struct {
  int flags;
  int stacksize;
} taskattr_t;


struct lpel_taskreq_t {
  unsigned int    uid;
  lpel_taskfunc_t func;
  void *          inarg;
  taskattr_t      attr;
  /* for joining: */
  /*
  atomic_t refcnt;
  task_t *waiter;
  void **joinval;
  */
};



/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  lpel_task_t *prev, *next;
  unsigned int uid;    /** unique identifier */
  taskattr_t attr;     /** attributes */
  taskstate_t state;   /** state */
  taskstate_blocked_t blocked_on; /** on which event the task is waiting */

  lpel_taskreq_t *request;      /** task request that created this task */
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
  pthread_mutex_t lock; /** context of the task*/
  coroutine_t ctx;      /** context of the task*/
  lpel_taskfunc_t code; /** function of the task */
  void *inarg;          /** input argument  */
};



lpel_task_t *_LpelTaskCreate( void);
void         _LpelTaskDestroy( lpel_task_t *t);

void _LpelTaskReset( lpel_task_t *t, lpel_taskreq_t *req);
void _LpelTaskUnset( lpel_task_t *t);

void _LpelTaskCall(  lpel_task_t *t);
void _LpelTaskBlock( lpel_task_t *ct, taskstate_blocked_t block_on);


#endif /* _TASK_H_ */
