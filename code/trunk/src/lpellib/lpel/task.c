
#include <malloc.h>
#include <assert.h>

#include "arch/atomic.h"
#include "arch/timing.h"

#include "lpel.h"
#include "task.h"

#include "worker.h"
#include "stream.h"

#include "monitoring.h"



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
static void TaskStartup(void *data);


lpel_taskreq_t *LpelTaskRequest( lpel_taskfunc_t func,
    void *inarg, int flags, int stacksize)
{
  lpel_taskreq_t *req = (lpel_taskreq_t *) malloc( sizeof(lpel_taskreq_t));
  
  req->uid = fetch_and_inc( &taskseq);
  req->func = func;
  req->inarg = inarg;
  req->attr.flags = flags;
  req->attr.stacksize = stacksize;
  /*TODO joinable: atomic init refcount */
  return req;
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void LpelTaskExit( lpel_task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  ct->state = TASK_ZOMBIE;
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
void LpelTaskYield( lpel_task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  ct->state = TASK_READY;
  /* context switch */
  co_resume();
}

unsigned int LpelTaskGetUID( lpel_task_t *t)
{
  return t->uid;
}

unsigned int LpelTaskReqGetUID( lpel_taskreq_t *t)
{
  return t->uid;
}


/******************************************************************************/
/* HIDDEN FUNCTIONS                                                           */
/******************************************************************************/

/**
 * Create a task
 */
lpel_task_t *_LpelTaskCreate( void)
{
  lpel_task_t *t = (lpel_task_t *) malloc( sizeof( lpel_task_t));
  pthread_mutex_init( &t->lock, NULL);
  
  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);
  t->state = TASK_CREATED;

  return t;
}


/**
 * Reset a task
 *
 */
void _LpelTaskReset( lpel_task_t *t, lpel_taskreq_t *req)
{
  assert( t->state == TASK_CREATED);

  /* copy request data */
  t->uid =   req->uid;
  t->code  = req->func;
  t->inarg = req->inarg;
  t->attr =  req->attr;


  t->prev = t->next = NULL;
  /* fix attributes */
  if (t->attr.stacksize <= 0) {
    t->attr.stacksize = TASK_ATTR_STACKSIZE_DEFAULT;
  }

  /*TODO if (JOINABLE) set t->request = req, else:*/
  {
    t->request = NULL;
    free( req);
  }

  if (t->attr.flags & LPEL_TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP(&t->times.creat);
  }
  t->cnt_dispatch = 0;
  t->dirty_list = STDESC_DIRTY_END; /* empty dirty list */
  atomic_set( &t->poll_token, 0); /* reset poll token to 0 */
  
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create( TaskStartup, (void *)t, NULL, t->attr.stacksize);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }

  t->state = TASK_READY;
}


/**
 * Unset the task after exiting on a worker
 */
void _LpelTaskUnset( lpel_task_t *t)
{
  assert( t->state == TASK_ZOMBIE);
  /* delete the coroutine */
  co_delete(t->ctx);
  t->state = TASK_CREATED;
}



/**
 * Destroy a task
 * - completely free the memory for that task
 */
void _LpelTaskDestroy( lpel_task_t *t)
{
  assert( t->state == TASK_ZOMBIE);
  atomic_destroy( &t->poll_token);
  /* destroy lock */
  pthread_mutex_destroy( &t->lock);
  /* free the TCB itself*/
  free(t);
}





/**
 * Call a task (context switch to a task)
 *
 */
void _LpelTaskCall( lpel_task_t *t)
{
  pthread_mutex_lock( &t->lock);
  t->cnt_dispatch++;
  t->state = TASK_RUNNING;
      
  if (t->attr.flags & LPEL_TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP( &t->times.start);
  }

  /*
   * CONTEXT SWITCH
   *
   * CAUTION: A coroutine must not run simultaneously in more than one thread!
   */
  co_call( t->ctx);

  /* task returns in every case in a different state */
  assert( t->state != TASK_RUNNING);

  if (t->attr.flags & 
      (LPEL_TASK_ATTR_MONITOR_OUTPUT | LPEL_TASK_ATTR_COLLECT_TIMES)) {
    /* if monitor output, we need a timestamp */
    TIMESTAMP( &t->times.stop);
  }

#ifdef MONITORING_ENABLE
  /* output accounting info */
  if (t->attr.flags & LPEL_TASK_ATTR_MONITOR_OUTPUT) {
    _LpelMonitoringOutput( t->worker_context->mon, t);
  }
#endif
  pthread_mutex_unlock( &t->lock);
}


/**
 * Block a task
 */
void _LpelTaskBlock(lpel_task_t *ct, taskstate_blocked_t block_on)
{
  assert( ct->state == TASK_RUNNING);
  ct->state = TASK_BLOCKED;
  ct->blocked_on = block_on;
  /* context switch */
  co_resume();
}



/******************************************************************************/
/* PRIVATE FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Startup function for user specified task,
 * calls task function with proper signature
 *
 * @param data  the previously allocated lpel_task_t TCB
 */
static void TaskStartup( void *data)
{
  lpel_task_t *t = (lpel_task_t *)data;
  lpel_taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);
  /* if task function returns, exit properly */
  LpelTaskExit(t);
}


