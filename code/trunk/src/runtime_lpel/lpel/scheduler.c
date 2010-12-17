/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "bool.h"
#include "lpel.h"
#include "scheduler.h"
#include "timing.h"

#include "monitoring.h"

#include "atomic.h"
#include "taskqueue.h"

#include "task.h"
#include "stream.h"


struct schedcfg {
  int dummy;
};


struct schedctx {
  int wid; 
  lpelthread_t    *env;
  unsigned int     num_tasks;
  unsigned int     loop;
  taskqueue_t      queue;
  pthread_mutex_t  lock;
  pthread_cond_t   cond;
  bool             terminate;  
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
    TaskqueueInit( &sc->queue);
    pthread_mutex_init( &sc->lock, NULL);
    pthread_cond_init( &sc->cond, NULL);
    sc->terminate = false;

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
  schedctx_t *sc;

  /* wait for the workers to finish */
  for( i=0; i<num_workers; i++) {
    sc = &workers[i];
    pthread_mutex_destroy( &sc->lock);
    pthread_cond_destroy( &sc->cond);
    LpelThreadJoin( sc->env);
  }

  /* free memory for scheduler data */
  free( workers);
}



void SchedWakeup( task_t *by, task_t *whom)
{
  /* dependent on whom, put in global or wrapper queue */
  schedctx_t *sc = whom->sched_context;
  
  pthread_mutex_lock( &sc->lock );
  assert( sc->num_tasks > 0 );
  TaskqueuePushBack( &sc->queue, whom );
  if ( 1 == sc->queue.count ) {
    pthread_cond_signal( &sc->cond );
  }
  pthread_mutex_unlock( &sc->lock );
}



void SchedTerminate( void)
{
  int i;
  schedctx_t *sc;

  for( i=0; i<num_workers; i++) {
    sc = &workers[i];

    pthread_mutex_lock( &sc->lock );
    sc->terminate = true;
    pthread_cond_signal( &sc->cond );
    pthread_mutex_unlock( &sc->lock );
  }
}


/**
 * Assign a task to the scheduler
 */
void SchedAssignTask( task_t *t, int wid)
{
  schedctx_t *sc = &workers[wid];

  pthread_mutex_lock( &sc->lock );
  sc->num_tasks += 1;
  TaskqueuePushBack( &sc->queue, t );
  if ( 1 == sc->queue.count ) {
    pthread_cond_signal( &sc->cond );
  }
  pthread_mutex_unlock( &sc->lock );
}




/**
 * Main worker thread
 *
 */
static void SchedWorker( lpelthread_t *env, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;
  unsigned int delayed;
  task_t *t;  

  /* assign to core */
  LpelThreadAssign( env, sc->wid );

  /* MAIN SCHEDULER LOOP */
  sc->loop=0;
  do {
    pthread_mutex_lock( &sc->lock );
    while( 0 == sc->queue.count &&
        (sc->num_tasks > 0 || !sc->terminate) ) {
#ifdef MONITORING_ENABLE
      /* TODO only on flag */
      /*
      MonDebug( env, "wid %d waiting (queue: %u, tasks: %u, term: %d)\n",
          sc->wid, sc->queue.count, sc->num_tasks, sc->terminate);
      */
#endif
      pthread_cond_wait( &sc->cond, &sc->lock);
    }
    /* if queue is empty, t:=NULL */
    t = TaskqueuePopFront( &sc->queue);
    pthread_mutex_unlock( &sc->lock );

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
#ifdef MONITORING_ENABLE
      if ( t->attr.flags & TASK_ATTR_MONITOR ) {
        MonTaskPrint( &sc->env->mon, sc, t);
      }
#endif

      pthread_mutex_unlock( &t->lock);
      
      /* check state of task, place into appropriate queue */
      switch(t->state) {
        case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
          TaskDestroy( t);

          pthread_mutex_lock( &sc->lock );
          sc->num_tasks -= 1;
          pthread_mutex_unlock( &sc->lock );
          break;

        case TASK_BLOCKED: /* task returned from a blocking call*/
          /* do nothing */
          break;

        case TASK_READY: /* task yielded execution  */
          pthread_mutex_lock( &sc->lock );
          TaskqueuePushBack( &sc->queue, t );
          pthread_mutex_unlock( &sc->lock );
          break;

        default: assert(0); /* should not be reached */
      }
    } /* end if executed ready task */
    sc->loop++;
  } while ( t != NULL );

  assert( sc->num_tasks == 0);
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
#ifdef MONITORING_ENABLE
  /*
  MonDebug(env, "worker %d exited (queue: %u, tasks: %u, term: %d)\n",
      sc->wid, sc->queue.count, sc->num_tasks, sc->terminate);
  */
#endif
}



void SchedWrapper( lpelthread_t *env, void *arg)
{
  task_t *t = (task_t *)arg;
  schedctx_t *sc;

  /* create scheduler context */
  sc = (schedctx_t *) malloc( sizeof( schedctx_t));
  sc->num_tasks = 1;
  sc->wid = -1;
  sc->env = env;
  TaskqueueInit( &sc->queue);
  pthread_mutex_init( &sc->lock, NULL);
  pthread_cond_init( &sc->cond, NULL);
  sc->terminate = false;

  /* assign scheduler context */
  t->sched_context = sc;
  TaskqueuePushBack( &sc->queue, t);


  /* assign to others */
  LpelThreadAssign( env, -1 );

  /* MAIN SCHEDULER LOOP */
  sc->loop=0;
  do {
    pthread_mutex_lock( &sc->lock );
    while( 0 == sc->queue.count ) {
      pthread_cond_wait( &sc->cond, &sc->lock);
    }
    /* if queue is empty, t:=NULL */
    t = TaskqueuePopFront( &sc->queue);
    assert(t != NULL);
    pthread_mutex_unlock( &sc->lock );

    
    t->cnt_dispatch++;

    /* execute task */
    TIMESTAMP(&t->times.start);
    TaskCall(t);
    TIMESTAMP(&t->times.stop);
      
    /* task returns in every case in a different state */
    assert( t->state != TASK_RUNNING);

    /* output accounting info */
#ifdef MONITORING_ENABLE
      if ( t->attr.flags & TASK_ATTR_MONITOR ) {
        MonTaskPrint( &sc->env->mon, sc, t);
      }
#endif

    /* check state of task, place into appropriate queue */
    switch(t->state) {
      case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
        sc->terminate = true;
        sc->num_tasks -= 1;
        TaskDestroy( t);
        break;

      case TASK_BLOCKED: /* task returned from a blocking call*/
        /* do nothing */
        break;

      case TASK_READY: /* task yielded execution  */
        pthread_mutex_lock( &sc->lock );
        TaskqueuePushBack( &sc->queue, t );
        pthread_mutex_unlock( &sc->lock );
        break;

      default: assert(0); /* should not be reached */
    }
    sc->loop++;
  } while ( !sc->terminate );

  assert( sc->num_tasks == 0);
#ifdef MONITORING_ENABLE
  /*
  MonDebug(env, "wrapper exited.\n");
  */
#endif
  
  /* free the scheduler context */
  free( sc);
}




/****************************************************************************/
/*  Printing, for monitoring output                                         */
/****************************************************************************/

void SchedPrintContext( schedctx_t *sc, FILE *file)
{
  if ( sc->wid < 0) {
    (void) fprintf(file, "loop %u ", sc->loop);
  } else {
    (void) fprintf(file, "wid %d loop %u ", sc->wid, sc->loop);
  }
}

