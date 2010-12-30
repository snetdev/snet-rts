/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "arch/timing.h"
#include "arch/atomic.h"

#include "worker.h"

#include "task.h"
#include "threading.h"




static int num_workers = -1;
static workerctx_t *workers;
static workercfg_t  config;


/* worker thread function declaration */
static void *WorkerThread( void *arg);



/******************************************************************************/
/*  Convenience functions for sending messages between workers                */
/******************************************************************************/


static inline void SendTerminate( workerctx_t *target)
{
  workermsg_t msg;
  /* compose a task term message */
  msg.type = WORKER_MSG_TERMINATE;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}

static inline void SendWakeup( workerctx_t *target)
{
  workermsg_t msg;
  /* compose a task wakeup message */
  msg.type = WORKER_MSG_WAKEUP;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}

static inline void SendAssign( workerctx_t *target, lpel_taskreq_t *req)
{
  workermsg_t msg;
  /* compose a task assign message */
  msg.type = WORKER_MSG_ASSIGN;
  msg.m.treq = req;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}


/******************************************************************************/
/*  PUBLIC FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Assign a task to the worker
 */
void LpelWorkerTaskAssign( lpel_taskreq_t *t, int wid)
{
  workerctx_t *wc = &workers[wid];

  SendAssign( wc, t);
}

/**
 * Create a wrapper thread for a single task
 */
void LpelWorkerWrapperCreate( lpel_taskreq_t *t, char *name)
{
  assert(name != NULL);

  /* create worker context */
  workerctx_t *wc = (workerctx_t *) malloc( sizeof( workerctx_t));

  wc->wid = -1;
  wc->num_tasks = 0;
  wc->terminate = false;

  wc->sched = SchedCreate( -1);

  
  if (t->attr.flags & LPEL_TASK_ATTR_MONITOR_OUTPUT) {
    wc->mon = _LpelMonitoringCreate( config.node, name);
  } else {
    wc->mon = NULL;
  }

  /* taskqueue of free tasks */
  TaskqueueInit( &wc->free_tasks);

  /* mailbox */
  MailboxInit( &wc->mailbox);

  /* send assign message for the task */
  SendAssign( wc, t);

  /* send terminate message afterwards, as this is the only task */
  SendTerminate( wc);


 (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
 (void) pthread_detach( wc->thread);
}


/**
 * Send termination message to all workers
 */
void LpelWorkerTerminate(void)
{
  int i;
  workerctx_t *wc;

  /* signal the workers to terminate */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    
    /* send a termination message */
    SendTerminate( wc);
  }

}


/******************************************************************************/
/*  HIDDEN FUNCTIONS                                                          */
/******************************************************************************/


/**
 * Initialise worker globally
 *
 *
 * @param size  size of the worker set, i.e., the total number of workers
 * @param cfg   additional configuration
 */
void _LpelWorkerInit(int size, workercfg_t *cfg)
{
  int i;

  assert(0 <= size);
  num_workers = size;

  /* handle configuration */
  if (cfg != NULL) {
    /* copy configuration for local use */
    config = *cfg;
  } else {
    config.node = -1;
    config.do_print_workerinfo = true;
  }

  /* allocate the array of worker contexts */
  workers = (workerctx_t *) malloc( num_workers * sizeof(workerctx_t) );

  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    char wname[11];
    wc->wid = i;
    wc->num_tasks = 0;
    wc->terminate = false;

    wc->sched = SchedCreate( i);

    snprintf( wname, 11, "worker%02d", i);
    wc->mon = _LpelMonitoringCreate( config.node, wname);
    
    /* taskqueue of free tasks */
    TaskqueueInit( &wc->free_tasks);

    /* mailbox */
    MailboxInit( &wc->mailbox);

    /* spawn joinable thread */
    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
  }
}



/**
 * Cleanup worker contexts
 *
 */
void _LpelWorkerCleanup(void)
{
  int i;
  workerctx_t *wc;

  /* wait on workers */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    /* wait for the worker to finish */
    (void) pthread_join( wc->thread, NULL);
  }
  /* cleanup the mailbox */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    MailboxCleanup( &wc->mailbox);
  }

  /* free memory */
  free( workers);
}


/**
 * Wakeup a task from within another task - this internal function
 * is used from within StreamRead/Write/Poll
 * TODO: wakeup from without a task, i.e. from kernel by an asynchronous
 *       interrupt for a completed request?
 *       -> by == NULL, at least there is no valid by->worker_context
 */
void _LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom)
{
  int was_empty;
  /* worker context of the task to be woken up */
  workerctx_t *wc = whom->worker_context;

  /* NOTE: do NEVER whom->state = TASK_READY;
   * as the state is private to that task, and only needed for Reschedule()
   * to determine what to do with it. It can happen that the task has not
   * returned yet while in TASK_WAITING, hence reschedule would also read
   * then TASK_READY and effectively we end up trying to put the task in
   * in the sched structure twice.
   */
  was_empty = SchedMakeReady( wc->sched, whom);
  /*
   * if the runqueue (sched) was empty AND
   * the task to be woken belongs to a different thread
   */
  if ( was_empty && ( !by || (wc != by->worker_context))) {
    SendWakeup( wc);
  }
}









/******************************************************************************/
/*  PRIVATE FUNCTIONS                                                         */
/******************************************************************************/


static lpel_task_t *AssignTask( workerctx_t *wc, lpel_taskreq_t *req)
{
  lpel_task_t *task;
  if (wc->free_tasks.count > 0) {
    task = TaskqueuePopFront( &wc->free_tasks);
  } else {
    task = _LpelTaskCreate();
  }
  _LpelTaskReset( task, req);
  /* assign worker_context */
  task->worker_context = wc;
  return task;
}

static void ReturnTask( workerctx_t *wc, lpel_task_t *t)
{
  _LpelTaskUnset( t);
  /* un-assign worker_context */
  t->worker_context = NULL;

  TaskqueuePushFront( &wc->free_tasks, t);
}

/**
 * Reschedule workers own task, returning from a call
 */
static void RescheduleTask( lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;
  /* reschedule task */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      ReturnTask( wc, t);
      wc->num_tasks -= 1;
      break;
    case TASK_BLOCKED: /* task returned from a blocking call*/
      /* do nothing */
      break;
    case TASK_READY: /* task yielded execution  */
      SchedMakeReady( wc->sched, t);
      break;
    default: assert(0); /* should not be reached */
  }
}


static void ProcessMessage( workerctx_t *wc, workermsg_t *msg)
{
  lpel_task_t *task;

  switch( msg->type) {
    case WORKER_MSG_WAKEUP:
      /* worker has new ready tasks,
       * just wakeup to continue loop
       */
      break;
    case WORKER_MSG_TERMINATE:
      wc->terminate = true;
      break;
    case WORKER_MSG_ASSIGN:
      wc->num_tasks += 1;
      /* assign task to worker */
      task = AssignTask( wc, msg->m.treq);
      (void) SchedMakeReady( wc->sched, task);
      break;
    default: assert(0);
  }
}


static void FetchAllMessages( workerctx_t *wc)
{
  workermsg_t msg;
  while( MailboxHasIncoming( &wc->mailbox) ) {
    MailboxRecv( &wc->mailbox, &msg);
    ProcessMessage( wc, &msg);
  }
}


/**
 * Worker loop
 */
static void WorkerLoop( workerctx_t *wc)
{
  lpel_task_t *t;

  wc->loop = 0;
  do {
    t = SchedFetchReady( wc->sched);
    if (t != NULL) {
      /* increment counter for next loop */
      wc->loop++;

      /* execute task */
      _LpelTaskCall( t);
      
      RescheduleTask( t);
    } else {
      /* wait for a new message and process */
      workermsg_t msg;
      MailboxRecv( &wc->mailbox, &msg);
      ProcessMessage( wc, &msg);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !(wc->num_tasks==0 && wc->terminate) );

  //_LpelMonitoringDebug( wc->mon, "Worker exited.\n");
}



/**
 * Thread function for workers (and wrappers)
 */
static void *WorkerThread( void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;
  
  /* Init libPCL */
  co_thread_init();

  /* assign to cores */
  _LpelThreadAssign( wc->wid);

  /* call worker loop */
  WorkerLoop( wc);
  
  /* cleanup monitoring */
  if (wc->mon) _LpelMonitoringDestroy( wc->mon);
  
  SchedDestroy( wc->sched);

  /* destroy all the free tasks */
  while( wc->free_tasks.count > 0) {
    lpel_task_t *t = TaskqueuePopFront( &wc->free_tasks);
    free( t);
  }

  /* on a wrapper, we also can cleanup more*/
  if (wc->wid < 0) {
    /* clean up the mailbox for the worker */
    MailboxCleanup( &wc->mailbox);

    /* free the worker context */
    free( wc);
  }
  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
}

