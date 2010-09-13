#ifndef _TASK_H_
#define _TASK_H_

#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "timing.h"
#include "atomic.h"

/**
 * If a stacksize attribute <= 0 is specified,
 * use the default stacksize
 */
#define TASK_STACKSIZE_DEFAULT  8192  /* 8k stacksize*/



#define TASK_ATTR_DEFAULT      (0)
#define TASK_ATTR_MONITOR   (1<<0)
#define TASK_ATTR_WAITANY   (1<<1)
#define TASK_ATTR_SYSTEM    (1<<8)


#define BIT_IS_SET(vec,b)   (( (vec) & (b) ) != 0 )

/**
 * Check if a task is a waitany-task
 * @param t   pointer to task_t
 */
#define TASK_IS_WAITANY(t)  (BIT_IS_SET((t)->attr.flags, TASK_ATTR_WAITANY))


struct stream_mh;
struct buffer;


typedef enum {
  TASK_INIT    = 'I',
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_WAITING = 'W',
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
  unsigned long uid;
  taskstate_t state;
  /* queue handling: prev, next */
  task_t *volatile prev;
  task_t *volatile next;

  /* attributes */
  taskattr_t attr;

  /* pointer to signalling flag
   * pointer to void*
   * contents can change arbitrarily -> volatile
   */
  volatile void **event_ptr;
  taskstate_wait_t wait_on;
  struct buffer *wait_s;

  /* waitany-task specific stuff */
  volatile void* wany_flag;

  /* reference counter */
  atomic_t refcnt;

  int owner;         /* owning worker thread TODO as place_t */
  void *sched_info;  /* scheduling information  */

  /* ACCOUNTING INFORMATION */
  /* timestamps for creation, start/stop of last dispatch */
  struct {
    timing_t creat, start, stop;
  } times;
  /* dispatch counter */
  unsigned long cnt_dispatch;
  /* streams marked as dirty */
  struct stream_mh *dirty_list;

  /* CODE */
  coroutine_t ctx;
  taskfunc_t code;
  void *inarg;  /* input argument  */
  void *outarg; /* output argument */
};



extern task_t *TaskCreate( taskfunc_t, void *inarg, taskattr_t attr);
extern int TaskDestroy(task_t *t);

struct stream_mh **TaskGetDirtyStreams( task_t *ct);

extern void TaskCall(task_t *ct);
extern void TaskWaitOnRead( task_t *ct, struct buffer *s);
extern void TaskWaitOnWrite( task_t *ct, struct buffer *s);
extern void TaskWaitOnAny(task_t *ct);
extern void TaskExit(task_t *ct, void *outarg);
extern void TaskYield(task_t *ct);




#endif /* _TASK_H_ */
