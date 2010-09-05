
#include <malloc.h>
#include <assert.h>

#include "lpel.h"
#include "task.h"

#include "atomic.h"
#include "timing.h"

#include "stream.h"



/**
 * 2 to the power of following constant
 * determines the initial group capacity of streamsets
 * in tasks that can wait on any input
 */
#define TASK_WAITANY_GRPS_INIT  2



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
static void TaskStartup(void *data);


/**
 * Create a task
 */
task_t *TaskCreate( taskfunc_t func, void *inarg, taskattr_t attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->uid = fetch_and_inc(&taskseq);
  t->state = TASK_INIT;
  t->prev = t->next = NULL;
  
  /* task attributes */
  t->attr = attr;
  /* fix attributes */
  if (t->attr.stacksize <= 0) {
    t->attr.stacksize = TASK_STACKSIZE_DEFAULT;
  }

  /* initialize reference counter to 1*/
  atomic_set(&t->refcnt, 1);

  t->event_ptr = NULL;
  
  //t->owner = -1;
  t->sched_info = NULL;

  TIMESTAMP(&t->times.creat);

  t->cnt_dispatch = 0;

  /* init streamset to write */
  StreamtabInit( &t->streams_write, 0);

  /* init streamset to read */
  StreamtabInit( &t->streams_read,
      (TASK_IS_WAITANY(t)) ?
      TASK_WAITANY_GRPS_INIT : 0
      );


  /* other stuff that is special for WAIT_ANY tasks */
  if ( TASK_IS_WAITANY(t) ) {
    /* allocate specific info struct */
    t->waitany_info = (struct waitany *) malloc( sizeof(struct waitany) );

    RwlockInit( &t->waitany_info->rwlock, LpelNumWorkers() );
    FlagtreeInit(
        &t->waitany_info->flagtree,
        TASK_WAITANY_GRPS_INIT,
        &t->waitany_info->rwlock
        );
    /* max_grp_idx = 2^x - 1 */
    t->waitany_info->max_grp_idx = (1<<TASK_WAITANY_GRPS_INIT)-1;
  }

  t->code = func;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create(TaskStartup, (void *)t, NULL, t->attr.stacksize);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
  }
  t->inarg = inarg;
  t->outarg = NULL;
 
  return t;
}


/**
 * Destroy a task
 * @return 1 if the task was physically freed (refcnt reached 0)
 */
int TaskDestroy(task_t *t)
{
  /* only if nothing references the task anymore */
  /* if ( fetch_and_dec(&t->refcnt) == 1) { */
  if ( atomic_dec(&t->refcnt) == 0) {

    /* free the streamsets */
    StreamtabCleanup(&t->streams_write);
    StreamtabCleanup(&t->streams_read);

    /* waitany-specific cleanup */
    if ( TASK_IS_WAITANY(t) ) {
      RwlockCleanup( &t->waitany_info->rwlock );
      FlagtreeCleanup( &t->waitany_info->flagtree );
      /* free waitany struct */
      free( t->waitany_info );
    }

    /* delete the coroutine */
    co_delete(t->ctx);

    /* free the TCB itself*/
    free(t);

    return 1;
  }
  return 0;
}


/**
 * Call a task (context switch to a task)
 *
 * @pre t->state == TASK_READY
 */
void TaskCall(task_t *t)
{
  assert( t->state == TASK_READY );
  t->state = TASK_RUNNING;
  co_call(t->ctx);
}



/**
 * Hidden Startup function for user specified task function
 *
 * Calls task function with proper signature
 */
static void TaskStartup(void *data)
{
  task_t *t = (task_t *)data;
  taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);

  /* if task function returns, exit properly */
  TaskExit(t, NULL);
}



/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnRead(task_t *ct, stream_t *s)
{
  assert( ct->state == TASK_RUNNING );
  
  /* WAIT on read event*/;
  ct->event_ptr = (int *) &s->buf[s->pwrite];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_READ;
  ct->wait_s = s;
  
  /* context switch */
  co_resume();
}

/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnWrite(task_t *ct, stream_t *s)
{
  assert( ct->state == TASK_RUNNING );

  /* WAIT on write event*/
  ct->event_ptr = (int *) &s->buf[s->pread];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_WRITE;
  ct->wait_s = s;
  
  /* context switch */
  co_resume();
}

/**
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnAny(task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  assert( TASK_IS_WAITANY(ct) );

  /* WAIT upon any input stream setting root flag */
  ct->event_ptr = &ct->waitany_info->flagtree.buf[0];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_ANY;
  ct->wait_s = NULL;
  
  /* context switch */
  co_resume();
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @param outarg  join argument
 * @pre ct->state == TASK_RUNNING
 */
void TaskExit(task_t *ct, void *outarg)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_ZOMBIE;
  
  ct->outarg = outarg;
  /* context switch */
  co_resume();

  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to worker thread
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskYield(task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  /* context switch */
  co_resume();
}



/**
 * Join with a task
 */
/*TODO place in waiting queue with event_ptr pointing to task's outarg */

