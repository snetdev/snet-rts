#ifndef _TASK_H_
#define _TASK_H_

#include <stdio.h>
#include <pthread.h>
#include <pcl.h>    /* tasks are executed in user-space with help of
                       GNU Portable Coroutine Library  */

#include "scheduler.h"
#include "timing.h"
#include "atomic.h"


/**
 * If a stacksize attribute <= 0 is specified,
 * use the default stacksize
 */
#define TASK_STACKSIZE_DEFAULT  8192  /* 8k stacksize*/



#define TASK_ATTR_DEFAULT      (0)
#define TASK_ATTR_MONITOR   (1<<0)

#define TASK_PRINT_TIMES    (1<<0)
#define TASK_PRINT_STREAMS  (1<<1)


struct stream_desc;
struct stream;


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


typedef struct task task_t;

typedef void (*taskfunc_t)(task_t *self, void *inarg);

typedef struct {
  int flags;
  int stacksize;
} taskattr_t;

/*
 * TASK CONTROL BLOCK
 */
struct task {
  task_t *prev, *next;  /** intrinsic pointers for organizing tasks in a list*/
  /*task_t *volatile prev;*/
  /*task_t *volatile next;*/
  unsigned long uid;    /** unique identifier */
  taskattr_t attr;      /** attributes */
  taskstate_t state;    /** state */
  taskstate_wait_t wait_on; /** on which event the task is waiting */
  /**
   * lock to protect a task from concurrent execution and manipulation
   */
  pthread_mutex_t lock;
  schedctx_t *sched_context;  /** scheduling context for this task */
  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  struct stream_desc *wakeup_sd;
  atomic_t poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  /** timestamps for creation, start/stop of last dispatch */
  struct {
    timing_t creat, start, stop;
  } times;
  /* dispatch counter */
  unsigned long cnt_dispatch;
  /* streams marked as dirty */
  struct stream_desc *dirty_list;

  /* CODE */
  coroutine_t ctx;
  taskfunc_t code;
  void *inarg;  /* input argument  */
};



extern task_t *TaskCreate( taskfunc_t, void *inarg, taskattr_t *attr);

extern void TaskCall(task_t *ct);
extern void TaskExit(task_t *ct);
extern void TaskYield(task_t *ct);


extern void TaskDestroy(task_t *t);

extern void TaskPrint( task_t *t, FILE *file, int flags);

#endif /* _TASK_H_ */
