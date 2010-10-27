
#include <malloc.h>
#include <assert.h>

#include "lpel.h"
#include "task.h"

#include "atomic.h"
#include "timing.h"

#include "stream.h"




static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
static void TaskStartup(void *data);


/**
 * Create a task
 */
task_t *TaskCreate( taskfunc_t func, void *inarg, taskattr_t *attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->uid = fetch_and_inc(&taskseq);
  t->state = TASK_INIT;
  t->prev = t->next = NULL;
  
  /* task attributes */
  t->attr = *attr;
  /* fix attributes */
  if (t->attr.stacksize <= 0) {
    t->attr.stacksize = TASK_STACKSIZE_DEFAULT;
  }

  /* initialize reference counter to 1*/
  atomic_set(&t->refcnt, 1);

  atomic_set(&t->poll_token, 0);
  
  t->sched_context = NULL;
  t->sched_info = NULL;

  TIMESTAMP(&t->times.creat);

  t->cnt_dispatch = 0;

  /* init streamset to write */
  t->dirty_list = (stream_desc_t *)-1;
  

  t->code = func;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create(TaskStartup, (void *)t, NULL, t->attr.stacksize);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }
  pthread_mutex_init( &t->lock, NULL);
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


    pthread_mutex_destroy( &t->lock);

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
  assert( t->state != TASK_RUNNING);
  t->state = TASK_RUNNING;

  /* CAUTION: a coroutine must not run simultaneously in more than one thread! */
  /* CONTEXT SWITCH */
  co_call(t->ctx);
}





/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @param outarg  join argument
 * @pre ct->state == TASK_RUNNING
 */
void TaskExit( task_t *ct, void *outarg)
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
 * Yield execution back to scheduler voluntarily
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskYield( task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  /* context switch */
  co_resume();
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


