/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>
#include <pcl.h>

#include "arch/atomic.h"

#include "worker.h"

#include "task.h"
#include "threading.h"



static int num_workers = -1;
static workerctx_t *workers;
static workercfg_t  config;



static atomic_t glob_task_count = ATOMIC_INIT(0);



/* worker thread function declaration */
static void *WorkerThread( void *arg);


static void RescheduleTask( workerctx_t *wc, lpel_task_t *t);
static void FetchAllMessages( workerctx_t *wc);


static inline void TaskAdd( void)
{
  (void) fetch_and_inc( &glob_task_count);
}

static inline void TaskRemove( void)
{
  int i;
  workerctx_t *wc;
  workermsg_t msg;

  /* compose a task term message */
  msg.type = WORKER_MSG_TERMINATE;

  if (1 == fetch_and_dec( &glob_task_count)) {
    /* signal the workers to terminate */
    for( i=0; i<num_workers; i++) {
      wc = &workers[i];
      
      /* send */
      MailboxSend( &wc->mailbox, &msg);
    }
  }
}

/******************************************************************************/
/*  Convenience functions for sending messages between workers                */
/******************************************************************************/


static inline void SendAssign( workerctx_t *target, lpel_taskreq_t *req)
{
  workermsg_t msg;
  /* compose a task assign message */
  msg.type = WORKER_MSG_ASSIGN;
  msg.body.treq = req;


  /* send */
  MailboxSend( &target->mailbox, &msg);
}



static inline void SendWakeup( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task wakeup message */
  msg.type = WORKER_MSG_WAKEUP;
  msg.body.task = t;
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

  wc->terminate = false;

  wc->wait_cnt = 0;
  TimingZero( &wc->wait_time);

  /* Wrapper is excluded from scheduling module */
  wc->sched = NULL;
  wc->wraptask = NULL;

  
  if (t->in.flags & LPEL_TASK_ATTR_MONITOR_OUTPUT) {
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

 (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
 (void) pthread_detach( wc->thread);
}


/**
 * Send termination message to all workers
 */
void LpelWorkerTerminate(void)
{

}


/******************************************************************************/
/*  HIDDEN FUNCTIONS                                                          */
/******************************************************************************/




void _LpelWorkerFinalizeTask( workerctx_t *wc)
{
  if (wc->predecessor != NULL) {
    _LpelMonitoringDebug( wc->mon, "Finalize for task %d.\n", wc->predecessor->uid);

    TaskUnlock( wc->predecessor );
    RescheduleTask( wc, wc->predecessor);

    wc->predecessor = NULL;
  } else {
    _LpelMonitoringDebug( wc->mon, "No finalize! no predecessor on worker %d!\n", wc->wid);
  }
}

void TaskCall( workerctx_t *wc, lpel_task_t *t)
{
  /* count how often _LpelWorkerDispatcher has executed a task */
  wc->loop++;
  _LpelMonitoringDebug( wc->mon, "Calling task %d.\n", t->uid);
  if (wc->predecessor != NULL) {
    _LpelMonitoringDebug( wc->mon, "Predecessor: task %d.\n", wc->predecessor->uid);
  } else {
    _LpelMonitoringDebug( wc->mon, "Predecessor: NO PRED!\n");
  }

  TaskLock( t);
  t->worker_context = wc;
  co_call( t->mctx);
}

void _LpelWorkerDispatcher( lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;

  /* set this task as predecessor on worker, for finalization*/
  wc->predecessor = t;
  _LpelMonitoringDebug( wc->mon, "TaskStop: setting predecessor for me %d.\n", t->uid);

  /* dependent of worker or wrapper */
  if (wc->wid != -1) {
    lpel_task_t *next;

    FetchAllMessages( wc);
    
    next = SchedFetchReady( wc->sched);
    if (next != NULL) {
      /* short circuit */
      if (next==t) return;

      /* execute task */
      TaskCall( wc, next);
    } else {
      /* no ready task! -> back to worker context */
      co_call( wc->mctx);
    }
    /*********************************
     * ... CTX SWITCH ...
     *********************************/
    _LpelWorkerFinalizeTask( wc);

  } else {
    /* we are on a wrapper.
     * back to wrapper context
     */
    co_call( wc->mctx);
  }
  /* let task continue it's business ... */
}





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

  /* prepare data structures */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    char wname[11];
    wc->wid = i;

    wc->wait_cnt = 0;
    TimingZero( &wc->wait_time);

    wc->terminate = false;

    wc->sched = SchedCreate( i);
    wc->wraptask = NULL;

    snprintf( wname, 11, "worker%02d", i);
    wc->mon = _LpelMonitoringCreate( config.node, wname);
    
    /* taskqueue of free tasks */
    TaskqueueInit( &wc->free_tasks);

    /* mailbox */
    MailboxInit( &wc->mailbox);
  }

  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
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
  /* cleanup the data structures */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    MailboxCleanup( &wc->mailbox);
    SchedDestroy( wc->sched);
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
  /* worker context of the task to be woken up */
  workerctx_t *wc = whom->worker_context;

  if (wc->wid < 0) {
    SendWakeup( wc, whom);
  } else {
    if ( !by || (by->worker_context != whom->worker_context)) {
      SendWakeup( wc, whom);
    } else {
      (void) SchedMakeReady( wc->sched, whom);
    }
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
static void RescheduleTask( workerctx_t *wc, lpel_task_t *t)
{
  /* reschedule task */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      ReturnTask( wc, t);
      TaskRemove();
      _LpelMonitoringDebug( wc->mon, "Killed task %d.\n", t->uid);

      if (wc->wid < 0) {
        wc->terminate = true;
      }
      break;
    case TASK_BLOCKED: /* task returned from a blocking call*/
      /* do nothing */
      break;
    case TASK_READY: /* task yielded execution  */
      if (wc->wid < 0) {
        SendWakeup( wc, t);
      } else {
        SchedMakeReady( wc->sched, t);
      }
      break;
    default: assert(0); /* should not be reached */
  }
}



static void ProcessMessage( workerctx_t *wc, workermsg_t *msg)
{
  lpel_task_t *t;
  
  //_LpelMonitoringDebug( wc->mon, "worker %d processing msg %d\n", wc->wid, msg->type);

  switch( msg->type) {
    case WORKER_MSG_WAKEUP:
      /* worker has new ready tasks,
       * just wakeup to continue loop
       */
      t = msg->body.task;
      _LpelMonitoringDebug( wc->mon, "Received wakeup for %d.\n", t->uid);
      if (wc->wid < 0) {
        wc->wraptask = t;
      } else {
        SchedMakeReady( wc->sched, t);
      }
      break;
    case WORKER_MSG_TERMINATE:
      wc->terminate = true;
      break;
    case WORKER_MSG_ASSIGN:
      /* assign task to worker */
      t = AssignTask( wc, msg->body.treq);

      TaskAdd();
      _LpelMonitoringDebug( wc->mon, "Creating task %d.\n", t->uid);
      
      if (wc->wid < 0) {
        wc->wraptask = t;
      } else {
        SchedMakeReady( wc->sched, t);
      }
      break;
    default: assert(0);
  }
}


static void WaitForNewMessage( workerctx_t *wc)
{
  workermsg_t msg;
  timing_t wtm;

  wc->wait_cnt += 1;
  
  TimingStart( &wtm);
  MailboxRecv( &wc->mailbox, &msg);
  TimingEnd( &wtm);
  
  _LpelMonitoringDebug( wc->mon,
      "worker %d waited (%u) for %lu.%09lu\n",
      wc->wid,
      wc->wait_cnt,
      (unsigned long) wtm.tv_sec, wtm.tv_nsec
      );
  
  TimingAdd( &wc->wait_time, &wtm);
  
  ProcessMessage( wc, &msg);
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
  lpel_task_t *t = NULL;
  
  do {
    t = SchedFetchReady( wc->sched);
    if (t != NULL) {

      /* execute task */
      TaskCall( wc, t);
      
      _LpelMonitoringDebug( wc->mon, "Back on worker %d context.\n", wc->wid);

      assert( wc->predecessor != NULL);
      
      _LpelWorkerFinalizeTask( wc);
    } else {
      /* no ready tasks */
      WaitForNewMessage( wc);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !( 0==atomic_read(&glob_task_count) && wc->terminate) );
  //} while ( !wc->terminate);

}


static void WrapperLoop( workerctx_t *wc)
{
  lpel_task_t *t = NULL;
  do {
    t = wc->wraptask;
    if (t != NULL) {

      /* execute task */
      TaskCall( wc, t);

      wc->wraptask = NULL;
      assert( wc->predecessor != NULL);
      
      _LpelWorkerFinalizeTask( wc);
    } else {
      /* no ready tasks */
      WaitForNewMessage( wc);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !wc->terminate);
}


/**
 * Thread function for workers (and wrappers)
 */
static void *WorkerThread( void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;

  /* Init libPCL */
  co_thread_init();
  wc->mctx = co_current();


  /* no predecessor */
  wc->predecessor = NULL;

  /* assign to cores */
  _LpelThreadAssign( wc->wid);

  /* start message */
  _LpelMonitoringDebug( wc->mon, "Worker %d started.\n", wc->wid);

  /*******************************************************/
  if ( wc->wid >= 0) {
    WorkerLoop( wc);
  } else {
    WrapperLoop( wc);
  }
  /*******************************************************/
  
  /* exit message */
  _LpelMonitoringDebug( wc->mon,
    "Worker %d exited. wait_cnt %u, wait_time %lu.%09lu\n",
    wc->wid,
    wc->wait_cnt, 
    (unsigned long) wc->wait_time.tv_sec, wc->wait_time.tv_nsec
    );


  /* cleanup monitoring */
  if (wc->mon) _LpelMonitoringDestroy( wc->mon);
  

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

