
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
  t->dirty_list = (stream_mh_t *)-1;
  
  t->wany_flag = NULL;

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

    /* free the not closed stream_mh's */
    if (0 != StreamPrintDirty( t, NULL))
    {
      //TODO warning
      assert(0);
    }

    /* delete the coroutine */
    co_delete(t->ctx);

    /* free the TCB itself*/
    free(t);

    return 1;
  }
  return 0;
}


stream_mh_t **TaskGetDirtyStreams( task_t *ct)
{
  return &ct->dirty_list;
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
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnRead( task_t *ct, stream_t *s)
{ 
  assert( ct->state == TASK_RUNNING );
  
  /* WAIT on read event*/;
  ct->event_ptr = (volatile void**) BufferGetWptr( s);
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_READ;
  ct->wait_s = s;
  
  /* context switch */
  co_resume();
}

/**
 * Set Task waiting for a read event
 *
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnWrite( task_t *ct, stream_t *s)
{
  assert( ct->state == TASK_RUNNING );

  /* WAIT on write event*/
  ct->event_ptr = (volatile void**) BufferGetRptr( s);
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_WRITE;
  ct->wait_s = s;
  
  /* context switch */
  co_resume();
}

/**
 * @pre ct->state == TASK_RUNNING
 */
void TaskWaitOnAny( task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  /*XXX WAIT upon any input stream setting root flag
  ct->event_ptr = &ct->waitany_info->flagtree.buf[0];
  */
  /* WAIT upon any input stream setting waitany_flag */
  ct->event_ptr = &ct->wany_flag;
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
 * Yield execution back to worker thread
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
 * Join with a task
 */
/*TODO place in waiting queue with event_ptr pointing to task's outarg */

