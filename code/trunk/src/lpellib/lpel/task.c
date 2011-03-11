
#include <stdlib.h>
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

static inline void TaskStart( lpel_task_t *t);
static inline void TaskStop( lpel_task_t *t);
static inline void TaskBlock( lpel_task_t *t, taskstate_t state);


lpel_taskreq_t *LpelTaskRequest( lpel_taskfunc_t func,
    void *inarg, int flags, int stacksize, int prio)
{
  lpel_taskreq_t *req = (lpel_taskreq_t *) malloc( sizeof(lpel_taskreq_t));
  
  req->uid = fetch_and_inc( &taskseq);
  req->in.func = func;
  req->in.arg = inarg;
  req->in.flags = flags;
  req->in.stacksize = stacksize;
  req->in.prio = prio;

  if (req->in.flags & LPEL_TASK_ATTR_JOINABLE) {
    atomic_init( &req->join.refcnt, 2);
  }

  return req;
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void LpelTaskExit( lpel_task_t *ct, void *joinarg)
{
  assert( ct->state == TASK_RUNNING );

  /* handle joining */
  if ( TASK_FLAGS( ct, LPEL_TASK_ATTR_JOINABLE)) {
    ct->request->join.arg = joinarg;
    //WMB();
    if ( 1 == fetch_and_dec( &ct->request->join.refcnt)) {
      lpel_task_t *whom = ct->request->join.parent;
      assert( whom != NULL);
      _LpelWorkerTaskWakeup( ct, whom);
    }
  }

  TaskBlock( ct, TASK_ZOMBIE);
  /* execution never comes back here */
  assert(0);
}

/**
 * Join with child
 */
void* LpelTaskJoin( lpel_task_t *ct, lpel_taskreq_t *child)
{
  void* joinarg;

  assert( ct->state == TASK_RUNNING );

  /* NOTE: cannot check if child is joinable, because
   * if it is not, it might already have been freed
   */
  child->join.parent = ct;
  //WMB();
  if ( 1 != fetch_and_dec( &child->join.refcnt)) {
    _LpelTaskBlock( ct, BLOCKED_ON_CHILD);
  }
  /* store joinarg */
  joinarg = child->join.arg;
  /* joining on a non-joinable task will result in a double-free */
  atomic_destroy( &req->join.refcnt);
  free( child);

  return joinarg; 
}

/**
 * Yield execution back to scheduler voluntarily
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void LpelTaskYield( lpel_task_t *ct)
{
  TaskBlock( ct, TASK_READY);
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
  
  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);

#ifdef TASK_USE_SPINLOCK
  pthread_spin_init( &t->lock, PTHREAD_PROCESS_PRIVATE);
#else
  pthread_mutex_init( &t->lock, NULL);
#endif

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
  t->uid       =  req->uid;

  t->code      =  req->in.func;
  t->inarg     =  req->in.arg;
  t->flags     =  req->in.flags;
  t->stacksize =  req->in.stacksize;

  t->sched_info.prio = req->in.prio;

  if (t->stacksize <= 0) {
    t->stacksize = LPEL_TASK_ATTR_STACKSIZE_DEFAULT;
  }

  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_JOINABLE)) {
    t->request = req;
  } else {
    /* request is not needed anymore and that's
     * the earliest possible point to free it
     */
    t->request = NULL;
    atomic_destroy( &req->join.refcnt);
    free( req);
  }

  t->prev = t->next = NULL;


  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES)) {
    TIMESTAMP(&t->times.creat);
  }
  t->cnt_dispatch = 0;
  t->dirty_list = STDESC_DIRTY_END; /* empty dirty list */
  atomic_set( &t->poll_token, 0); /* reset poll token to 0 */
  
  /* function, argument (data), stack base address, stacksize */
  t->mctx = co_create( TaskStartup, (void *)t, NULL, t->stacksize);
  if (t->mctx == NULL) {
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
  co_delete(t->mctx);
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

#ifdef TASK_USE_SPINLOCK
  pthread_spin_destroy( &t->lock);
#else
  pthread_mutex_destroy( &t->lock);
#endif

  /* free the TCB itself*/
  free(t);
}





/**
 * Block a task
 */
void _LpelTaskBlock(lpel_task_t *ct, taskstate_blocked_t block_on)
{
  ct->blocked_on = block_on;
  TaskBlock( ct, TASK_BLOCKED);
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

  _LpelWorkerFinalizeTask( t->worker_context);
  TaskStart( t);

  /* call the task function with inarg as parameter */
  func(t, t->inarg);
  /* if task function returns, exit properly */
  LpelTaskExit(t, NULL);
}


static inline void TaskStart( lpel_task_t *t)
{
  /* start timestamp, dispatch counter, state */
  t->cnt_dispatch++;
  t->state = TASK_RUNNING;
      
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES)) {
    TIMESTAMP( &t->times.start);
  }
}

static inline void TaskStop( lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;
  assert( t->state != TASK_RUNNING);

  /* stop timestamp */
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES) ) {
    /* if monitor output, we need a timestamp */
    TIMESTAMP( &t->times.stop);
  }

  /* output accounting info */
  if ( TASK_FLAGS(t, LPEL_TASK_ATTR_MONITOR_OUTPUT)) {
    _LpelMonitoringOutput( wc->mon, t);
  }

}

static inline void TaskBlock( lpel_task_t *t, taskstate_t state)
{

  assert( t->state == TASK_RUNNING);
  assert( state == TASK_READY || state == TASK_ZOMBIE || state == TASK_BLOCKED);

  /* set new state */
  t->state = state;

  TaskStop( t);
  _LpelWorkerDispatcher( t);
  TaskStart( t);
}
