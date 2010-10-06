/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>

#include "lpel.h"
#include "scheduler.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"
#include "taskqueue.h"

#include "stream.h"
#include "buffer.h"

struct schedcfg {
  int dummy;
};

typedef struct schedctx schedctx_t;

struct schedctx {
  int wid;
  taskqueue_t init_queue;
  pthread_mutex_t lock_iq;
  taskqueue_t ready_queue;
  struct {
    taskqueue_t on_read;
    taskqueue_t on_write;
    taskqueue_t on_any;
    int cnt;
  } waiting_queue;
  int cnt_tasks;
  int cnt_yield;
};

static int num_workers = -1;
static schedctx_t *schedarray;

static void ActivatorTask(task_t *self, void *inarg);

static void Reschedule(schedctx_t *sc, task_t *t);


/* prototypes for private functions */
static int CollectFromInit(schedctx_t *sc);

static bool WaitingTestOnRead(task_t *wt, void *arg);
static bool WaitingTestOnWrite(task_t *wt, void *arg);
static bool WaitingTestOnAny(task_t *wt, void *arg);


static void WaitingRemove(task_t *wt, void *arg);


/**
 * Initialise scheduler globally
 *
 * Call this function once before any other calls of the scheduler module
 *
 * @param size  size of the worker set, i.e., the total number of workers
 * @param cfg   additional configuration
 */
void SchedInit(int size, schedcfg_t *cfg)
{
  int i;
  schedctx_t *sc;

  assert(0 <= size);

  num_workers = size;
  
  /* allocate the array of scheduler data */
  schedarray = (schedctx_t *) calloc( size, sizeof(schedctx_t) );

  for (i=0; i<size; i++) {
      /* select from the previously allocated schedarray */
      sc = &schedarray[i];
      sc->wid = i;

      TaskqueueInit(&sc->init_queue);
      pthread_mutex_init( &sc->lock_iq, NULL );
      
      TaskqueueInit(&sc->ready_queue);
      
      TaskqueueInit(&sc->waiting_queue.on_read);
      TaskqueueInit(&sc->waiting_queue.on_write);
      TaskqueueInit(&sc->waiting_queue.on_any);
      sc->waiting_queue.cnt = 0;

      sc->cnt_tasks = 0;
  }
}


/**
 * Cleanup scheduler data
 *
 */
void SchedCleanup(void)
{
  int i;
  schedctx_t *sc;

  for (i=0; i<num_workers; i++) {
    sc = &schedarray[i];
    /* nothing to be done for taskqueue_t */

    /* destroy locks */
    pthread_mutex_destroy(&sc->lock_iq);
  }

  /* free memory for scheduler data */
  free(schedarray);
}


/**
 * Assign a task to the scheduler
 */
void SchedAssignTask(int wid, task_t *t)
{
  schedctx_t *sc = &schedarray[wid];

  /* place into the foreign init queue */
  pthread_mutex_lock( &sc->lock_iq );
  TaskqueueEnqueue( &sc->init_queue, t );
  pthread_mutex_unlock( &sc->lock_iq );
}





/**
 * Main scheduler task
 *
 * @pre mon != NULL
 */
void SchedTask(int wid, monitoring_t *mon)
{
  schedctx_t *sc = &schedarray[wid];
  unsigned int loop;
  task_t *act;  
  
  sc->cnt_yield = 0;

  /* create ActivatorTask */
  {
    taskattr_t tattr;
    tattr.flags = TASK_ATTR_SYSTEM;
    act = TaskCreate( ActivatorTask, sc, tattr);
    act->state = TASK_READY;
    /* put into ready queue */
    TaskqueueEnqueue( &sc->ready_queue, act );
  }

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    task_t *t;  

    /* fetch a task from the ready queue */
    t = TaskqueueDequeue( &sc->ready_queue );
    if (t != NULL) {
      /* EXECUTE TASK (context switch) */
      t->cnt_dispatch++;
      TIMESTAMP(&t->times.start);
      TaskCall(t);
      TIMESTAMP(&t->times.stop);
      /* task returns in every case with a different state */
      assert( t->state != TASK_RUNNING);

      /* output accounting info */
      if ( !BIT_IS_SET(t->attr.flags, TASK_ATTR_SYSTEM) ) {
        MonitoringPrint(mon, t);
        MonitoringDebug(mon, "(worker %d, loop %u)\n", wid, loop);
      }
      Reschedule(sc, t);
    } /* end if executed ready task */


    loop++;
  } while ( LpelWorkerTerminate() == 0 );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
  
  {
    double idle = (double)act->cnt_dispatch / (double)loop;
    MonitoringDebug(mon, "worker %d idle %f, yielded %d times\n", wid, idle, sc->cnt_yield);
  }
  
  /* destroy ActivatorTask */
  TaskDestroy(act);
}


/** PRIVATE FUNCTIONS *******************************************************/

/**
 * Activator task
 */
static void ActivatorTask(task_t *self, void *inarg)
{
  schedctx_t *sc = (schedctx_t *) inarg;
  int cnt;

  while (1) {
    /* fetch all tasks from init queue and insert into the ready queue */
    cnt = CollectFromInit(sc);
    /* update total number of tasks */
    sc->cnt_tasks += cnt;

    /* wakeup waiting tasks */
    if (sc->waiting_queue.cnt > 0) {
      if (sc->waiting_queue.on_read.count > 0) {
        cnt += TaskqueueIterateRemove(
            &sc->waiting_queue.on_read,
            WaitingTestOnRead,  /* on read test */
            WaitingRemove, (void*)sc
            );
      }
      if (sc->waiting_queue.on_write.count > 0) {
        cnt += TaskqueueIterateRemove(
            &sc->waiting_queue.on_write,
            WaitingTestOnWrite,  /* on write test */
            WaitingRemove, (void*)sc
            );
      }
      if (sc->waiting_queue.on_any.count > 0) {
        cnt += TaskqueueIterateRemove(
            &sc->waiting_queue.on_any,
            WaitingTestOnAny,  /* on any test */
            WaitingRemove, (void*)sc
            );
      }
    }

    /* yield kernel level thread */
    if (cnt == 0) {
      sched_yield();
      sc->cnt_yield++;
    }

    /* give control back to scheduler */
    TaskYield(self);
  } /* end while */
}

/**
 * Reschedule a task after execution
 *
 * @param sc   scheduler context
 * @param t    task returned from execution in a different state than READY
 */
static void Reschedule(schedctx_t *sc, task_t *t)
{
  /* check state of task, place into appropriate queue */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      /*TODO handle joinable tasks */
      LpelTaskRemove(t);

      /* decrement count of managed tasks */
      sc->cnt_tasks--;
      break;

    case TASK_WAITING: /* task returned from a blocking call*/
      {
        taskqueue_t *wq;
        switch (t->wait_on) {
          case WAIT_ON_READ:  wq = &sc->waiting_queue.on_read;  break;
          case WAIT_ON_WRITE: wq = &sc->waiting_queue.on_write; break;
          case WAIT_ON_ANY:   wq = &sc->waiting_queue.on_any;   break;
          default: assert(0);
        }
        /* put into appropriate waiting queue */
        TaskqueueEnqueue( wq, t );
        sc->waiting_queue.cnt++;
      }
      break;

    case TASK_READY: /* task yielded execution  */
      /* put into ready queue */
      TaskqueueEnqueue( &sc->ready_queue, t );
      break;

    default:
      assert(0); /* should not be reached */
  }
}



/**
 * Fetch all tasks from the init queue and enqueue them into the ready queue
 * @param sc    schedctx to operate on
 * @return      number of tasks moved
 */
static int CollectFromInit(schedctx_t *sc)
{
  int cnt;
  task_t *t;

  cnt = 0;
  pthread_mutex_lock( &sc->lock_iq );
  t = TaskqueueDequeue( &sc->init_queue );
  while (t != NULL) {
    //assert( t->state == TASK_INIT );
    //assert( t->owner == sc->wid );
    cnt++;
    /* set state to ready and place into the ready queue */
    t->state = TASK_READY;
    TaskqueueEnqueue( &sc->ready_queue, t );
    
    /* for next iteration: */
    t = TaskqueueDequeue( &sc->init_queue );
  }
  pthread_mutex_unlock( &sc->lock_iq );
  return cnt;
}


static bool WaitingTestOnRead(task_t *wt, void *arg)
{
  return BufferIsSpace( &wt->wait_s->buffer);
}

static bool WaitingTestOnWrite(task_t *wt, void *arg)
{
  return BufferTop( &wt->wait_s->buffer) != NULL;
}

/*XXX
static void WaitingTestGather(int i, void *arg)
{
  streamtab_t *tab = (streamtab_t *) arg;
  StreamtabChainAdd( tab, i );
}
*/

static bool WaitingTestOnAny(task_t *wt, void *arg)
{
  // assert( TASK_IS_WAITANY(wt) );
  //TODO exchange pointer size?
  // return xchg( (volatile void *) &wt->wany_flag, 0) != 0;
  if ( wt->wany_flag != 0 ) {
    wt->wany_flag = 0;
    return true;
  }
  return false;

/*XXX */
#if 0
  /* event_ptr points to the root of the flagtree */

  /* first of all, check root flag */
  if (*wt->event_ptr != 0) {
    int cnt;
    /* if root flag is set, try to gather all set leafs */
    StreamtabChainStart( &wt->streams_read );
    cnt = FlagtreeGather(
        &wt->waitany_info->flagtree,
        WaitingTestGather,
        &wt->streams_read
        );
    /* only return true, if at least on leaf could be gathered */
    /* return StreamtabChainNotEmpty( &wt->streams_read ); */
    return cnt > 0;
  }
  return false;
#endif
}


static void WaitingRemove(task_t *wt, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;

  wt->state  = TASK_READY;
  wt->wait_s = NULL;
  /*wt->event_ptr = NULL;*/
  sc->waiting_queue.cnt--;
  
  /* waiting queue contains only own tasks */
  TaskqueueEnqueue( &sc->ready_queue, wt );
}

