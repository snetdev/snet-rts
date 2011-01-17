/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

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

static inline void SendWakeup( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task wakeup message */
  msg.type = WORKER_MSG_WAKEUP;
  msg.body.task = t;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}

static inline void SendAssign( workerctx_t *target, lpel_taskreq_t *req)
{
  workermsg_t msg;
  /* compose a task assign message */
  msg.type = WORKER_MSG_ASSIGN;
  msg.body.treq = req;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}

static inline void SendRequestTask( workerctx_t *target, workerctx_t *from)
{
  workermsg_t msg;
  /* compose a request task message */
  msg.type = WORKER_MSG_REQUEST;
  msg.body.from_worker = from->wid;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}

static inline void SendTaskMigrate( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task migrate message */
  msg.type = WORKER_MSG_TASKMIG;
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
  wc->num_tasks = 0;
  wc->terminate = false;

  wc->wait_cnt = 0;
  TimingZero( &wc->wait_time);

  wc->sched = SchedCreate( -1);

  
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

  /* prepare data structures */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    char wname[11];
    wc->wid = i;
    wc->num_tasks = 0;

    wc->wait_cnt = 0;
    TimingZero( &wc->wait_time);
    wc->req_pending = 0;

    wc->terminate = false;

    wc->sched = SchedCreate( i);

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
  /* worker context of the task to be woken up */
  workerctx_t *wc = whom->worker_context;

  if (  !by || (wc != by->worker_context)) {
    SendWakeup( wc, whom);
  } else {
    assert( wc == by->worker_context);
    SchedMakeReady( wc->sched, whom);
  }
#if 0
  /* NOTE: do NEVER whom->state = TASK_READY;
   * as the state is private to that task, and only needed for Reschedule()
   * to determine what to do with it. It can happen that the task has not
   * returned yet while in TASK_WAITING, hence reschedule would also read
   * then TASK_READY and effectively we end up trying to put the task in
   * in the sched structure twice.
   */
  int was_empty = SchedMakeReady( wc->sched, whom);
  /*
   * if the runqueue (sched) was empty AND
   * the task to be woken belongs to a different thread
   */
  if ( was_empty && ( !by || (wc != by->worker_context))) {
    SendWakeup( wc, whom);
    //TODO this happens within a task!
    //_LpelMonitoringDebug( wc->mon, "worker wokeup %d\n", wc->wid);
  }
#endif
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

static void RequestTask( workerctx_t *wc)
{
  unsigned int i;
  timing_t min_wt = wc->wait_time;
  workerctx_t *wc_min_wt = wc;

  /* find worker with lowest wait_time */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wo = &workers[i];
    if ( wo->wait_time.tv_sec  < min_wt.tv_sec ||
         (wo->wait_time.tv_sec == min_wt.tv_sec && 
          wo->wait_time.tv_nsec < min_wt.tv_nsec   )) {
      min_wt = wo->wait_time;
      wc_min_wt = wo;
    }
  }

  if (wc_min_wt != wc && !wc_min_wt->terminate) {
    _LpelMonitoringDebug( wc->mon,
        "worker %d requesting task from worker %d\n",
        wc->wid, wc_min_wt->wid
        );
    /* send the message: to worker with minimal wait_time, from self=wc */
    SendRequestTask( wc_min_wt, wc);
    wc->req_pending = 1;
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
      task = msg->body.task;
      (void) SchedMakeReady( wc->sched, task);
      break;
    case WORKER_MSG_TERMINATE:
      wc->terminate = true;
      break;
    case WORKER_MSG_ASSIGN:
      wc->num_tasks += 1;
      /* assign task to worker */
      task = AssignTask( wc, msg->body.treq);
      (void) SchedMakeReady( wc->sched, task);
      break;
    case WORKER_MSG_TASKMIG:
      wc->req_pending = 0;
      task = msg->body.task;
      if (task != NULL) {
        assert( task->state != TASK_RUNNING);
        wc->num_tasks += 1;
        /* reassign new worker_context */
        task->worker_context = wc;
        (void) SchedMakeReady( wc->sched, task);
        _LpelMonitoringDebug( wc->mon,
            "worker %d received task %u\n",
            wc->wid, task->uid
            );
      }
      break;
    case WORKER_MSG_REQUEST:
      task = SchedFetchReady( wc->sched);
      if (task != NULL) {
        assert( task->state != TASK_RUNNING);
        wc->num_tasks -= 1;
        _LpelMonitoringDebug( wc->mon,
            "worker %d sending task %u to worker %d\n",
            wc->wid, task->uid, msg->body.from_worker
            );
      } else {
        _LpelMonitoringDebug( wc->mon,
            "worker %d sending no task to worker %d\n",
            wc->wid, msg->body.from_worker
            );
      }
      /* send a task to requesting worker */
      SendTaskMigrate( &workers[msg->body.from_worker], task);
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
  
  _LpelMonitoringDebug( wc->mon, "Worker %d started.\n", wc->wid);

  wc->loop = 0;
  do {
    t = SchedFetchReady( wc->sched);
    if (t != NULL) {
      /* increment counter for next loop */
      wc->loop++;

      /* execute task */
      _LpelTaskCall( t);

      /* output accounting info */
      if ( TASK_FLAGS(t, LPEL_TASK_ATTR_MONITOR_OUTPUT)) {
        _LpelMonitoringOutput( t->worker_context->mon, t);
      }
      
      RescheduleTask( t);
    } else {
      /* no ready tasks */
      workermsg_t msg;
      timing_t wtm;
//XXX XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
      /*
       * Very experimental: work-requesting.
       * Investigate, why #VCSes rockets to sky.
       */
      /* request a task if a real worker (no wrapper)*/
      //if ( wc->wid >= 0 && !wc->req_pending ) RequestTask( wc);
//XXX XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
      /* wait for a new message and process */
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
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !(wc->num_tasks==0 && wc->terminate) );

  _LpelMonitoringDebug( wc->mon,
    "Worker %d exited. wait_cnt %u, wait_time %lu.%09lu\n",
    wc->wid,
    wc->wait_cnt, 
    (unsigned long) wc->wait_time.tv_sec, wc->wait_time.tv_nsec
    );
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

