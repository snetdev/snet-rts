/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "lpel.h"
#include "scheduler.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"
#include "bqueue.h"

#include "task.h"
#include "stream.h"


struct schedcfg {
  int dummy;
};


struct schedctx {
  int wid; 
  unsigned int num_tasks;
  lpelthread_t *env;
  bqueue_t rq;
};

static int num_workers = -1;

static schedctx_t *workers;



/* static function prototypes */
static void SchedWorker( lpelthread_t *env, void *arg);


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

  assert(0 <= size);
  num_workers = size;
    
  /* allocate the array of scheduler data */
  workers = (schedctx_t *) malloc( num_workers * sizeof(schedctx_t) );

  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    schedctx_t *sc = &workers[i];
    char wname[11];
    sc->wid = i;
    sc->num_tasks = 0;
    BQueueInit( &sc->rq);
    snprintf( wname, 11, "worker%02d", i);
    /* aquire thread */ 
    sc->env = LpelThreadCreate( SchedWorker, (void*)sc, false, wname);
  }
}


/**
 * Cleanup scheduler data
 *
 */
void SchedCleanup(void)
{
  int i;

  /* wait for the workers to finish */
  for( i=0; i<num_workers; i++) {
    LpelThreadJoin( workers[i].env);
    BQueueCleanup( &workers[i].rq);
  }

  /* free memory for scheduler data */
  free( workers);
}



void SchedWakeup( task_t *by, task_t *whom)
{
  /* dependent on whom, put in global or wrapper queue */
  schedctx_t *sc = whom->sched_context;
  
  BQueuePut( &sc->rq, whom);
}



void SchedTerminate( void)
{
  int i;
  for( i=0; i<num_workers; i++) {
    BQueueTerm( &workers[i].rq ); 
  }
}


/**
 * Assign a task to the scheduler
 */
void SchedAssignTask( task_t *t, int wid)
{
  schedctx_t *sc = &workers[wid];

  sc->num_tasks += 1;
  BQueuePut( &sc->rq, t);
}




/**
 * Main worker thread
 *
 */
static void SchedWorker( lpelthread_t *env, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;
  unsigned int loop, delayed;
  task_t *t;  

  /* assign to core */
  LpelThreadAssign( env, sc->wid % num_workers );

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    t = BQueueFetch( &sc->rq);

    if (t != NULL) {
      /* aqiure task lock, count congestion */
      if ( EBUSY == pthread_mutex_trylock( &t->lock)) {
        delayed++;
        pthread_mutex_lock( &t->lock);
      }

      t->sched_context = sc;
      t->cnt_dispatch++;

      /* execute task */
      TIMESTAMP(&t->times.start);
      TaskCall(t);
      TIMESTAMP(&t->times.stop);
      
      /* task returns in every case in a different state */
      assert( t->state != TASK_RUNNING);

      /* output accounting info */
      MonTaskPrint( sc->env, t);
      MonDebug(env, "(worker %d, loop %u)\n", sc->wid, loop);

      /* check state of task, place into appropriate queue */
      switch(t->state) {
        case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
          TaskDestroy( t);
          sc->num_tasks -= 1;
          break;

        case TASK_WAITING: /* task returned from a blocking call*/
          /* do nothing */
          break;

        case TASK_READY: /* task yielded execution  */
          BQueuePut( &sc->rq, t);
          break;

        default: assert(0); /* should not be reached */
      }
      pthread_mutex_unlock( &t->lock);
    } /* end if executed ready task */
    loop++;
  } while ( t != NULL );

  assert( sc->num_tasks == 0);
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
  MonDebug(env, "worker %d exited.\n", sc->wid);
}



void SchedWrapper( lpelthread_t *env, void *arg)
{
  task_t *t = (task_t *)arg;
  unsigned int loop;
  bool terminate = false;
  schedctx_t *sc;

  /* create scheduler context */
  sc = (schedctx_t *) malloc( sizeof( schedctx_t));
  sc->num_tasks = 1;
  sc->wid = -1;
  sc->env = env;
  BQueueInit( &sc->rq);

  /* assign scheduler context */
  t->sched_context = sc;
  BQueuePut( &sc->rq, t);


  /* assign to others */
  LpelThreadAssign( env, -1 );

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    t = BQueueFetch( &sc->rq);
    assert(t != NULL);
    
    t->cnt_dispatch++;

    /* execute task */
    TIMESTAMP(&t->times.start);
    TaskCall(t);
    TIMESTAMP(&t->times.stop);
      
    /* task returns in every case in a different state */
    assert( t->state != TASK_RUNNING);

    /* output accounting info */
    MonTaskPrint( sc->env, t);
    MonDebug(env, "(loop %u)\n", loop);

    /* check state of task, place into appropriate queue */
    switch(t->state) {
      case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
        terminate = true;
        sc->num_tasks -= 1;
        TaskDestroy( t);
        break;

      case TASK_WAITING: /* task returned from a blocking call*/
        /* do nothing */
        break;

      case TASK_READY: /* task yielded execution  */
        BQueuePut( &sc->rq, t);
        break;

      default: assert(0); /* should not be reached */
    }
    loop++;
  } while ( !terminate );

  assert( sc->num_tasks == 0);
  MonDebug(env, "wrapper exited.\n");
  
  /* free the scheduler context */
  free( sc);
}


/** PRIVATE FUNCTIONS *******************************************************/

