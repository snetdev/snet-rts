/**
 * The LPEL scheduler containing code for workers and wrappers
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "bool.h"
#include "lpel.h"
#include "scheduler.h"
#include "timing.h"


#include "atomic.h"
#include "taskqueue.h"

#include "task.h"
#include "stream.h"


struct schedcfg {
  int node;
};


struct schedctx {
  int wid; 
  unsigned int     num_tasks;
  unsigned int     loop;
  taskqueue_t      queue;
  pthread_mutex_t  lock;
  pthread_cond_t   cond;
  bool             terminate;  
  pthread_t        thread;
  FILE            *monfile;
};

static int num_workers = -1;

static schedctx_t *workers;



static void *StartupThread( void *arg);



#define _MON_FNAME_MAXLEN   (LPEL_THREADNAME_MAXLEN + 12)
static void MonfileOpen( schedctx_t *sc, int node, char *name)
{
  char fname[_MON_FNAME_MAXLEN+1];

  if (node >= 0) {
    (void) snprintf(fname, _MON_FNAME_MAXLEN+1, "mon_n%02d_%s.log", node, name);
  } else {
    (void) snprintf(fname, _MON_FNAME_MAXLEN+1, "mon_%s.log", name);
  }
  fname[_MON_FNAME_MAXLEN] = '\0';

  /* open logfile */
  sc->monfile = fopen(fname, "w");
  assert( sc->monfile != NULL);
}

static void MonfileClose( schedctx_t *sc)
{
  if ( sc->monfile != NULL) {
    int ret;
    ret = fclose( sc->monfile);
    assert(ret == 0);
  }
}


static void MonfileDebug( schedctx_t *sc, const char *fmt, ...)
{
  timing_t ts;
  va_list ap;

  if ( sc->monfile == NULL) return;

  /* print current timestamp */
  TIMESTAMP(&ts);
  TimingPrint( &ts, sc->monfile);

  va_start(ap, fmt);
  vfprintf( sc->monfile, fmt, ap);
  fflush(sc->monfile);
  va_end(ap);
}


void SchedTaskPrint( task_t *t)
{
  timing_t ts;
  FILE *file = t->sched_context->monfile;

  if ( file == NULL) return;

  /* get task stop time */
  ts = t->times.stop;
  TimingPrint( &ts, file);

  //TODO check a flag
  //SchedPrintContext( sc, file);

  TaskPrint( t, file);

  fprintf( file, "\n");
}






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
    MonfileOpen( sc, -1, wname);
    /* spawn joinable thread */
    (void) pthread_create( &sc->thread, NULL, StartupThread, sc);
  }
}


/**
 * Create a wrapper thread for a single task
 */
void SchedWrapper(task_t *t, char *name)
{
  /* create scheduler context */
  schedctx_t *sc = (schedctx_t *) malloc( sizeof( schedctx_t));

  sc->num_tasks = 1;
  sc->wid = -1;
  TaskqueueInit( &sc->queue);
  pthread_mutex_init( &sc->lock, NULL);
  pthread_cond_init( &sc->cond, NULL);
  sc->terminate = false;

  /* assign scheduler context */
  t->sched_context = sc;
  TaskqueuePushBack( &sc->queue, t);

  MonfileOpen( sc, -1, name);

 (void) pthread_create( &sc->thread, NULL, StartupThread, sc);
 (void) pthread_detach( sc->thread);
}



void SchedTerminate(void)
{
  int i;
  schedctx_t *sc;

  /* signal the workers to terminate */
  for( i=0; i<num_workers; i++) {
    sc = &workers[i];
    
    pthread_mutex_lock( &sc->lock );
    sc->terminate = true;
    pthread_cond_signal( &sc->cond );
    pthread_mutex_unlock( &sc->lock );
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

  /* wait on workers */
  for( i=0; i<num_workers; i++) {
    sc = &workers[i];
    /* wait for the worker to finish */
    (void) pthread_join( sc->thread, NULL);
  }

  /* free memory for scheduler data */
  free( workers);
}



void SchedWakeup( task_t *by, task_t *whom)
{
  schedctx_t *sc = whom->sched_context;
  
  /* put in owner of whom's ready queue */
  pthread_mutex_lock( &sc->lock );
  assert( sc->num_tasks > 0 );
  TaskqueuePushBack( &sc->queue, whom );
  if ( 1 == sc->queue.count ) {
    pthread_cond_signal( &sc->cond );
  }
  pthread_mutex_unlock( &sc->lock );
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







/*****************************************************************************/
/*  WORKER and WRAPPER functions                                             */
/*****************************************************************************/


/**
 * Main worker function
 */
static void Worker( schedctx_t *sc)
{
  task_t *t;  

  /* MAIN SCHEDULER LOOP */
  sc->loop=0;
  do {
    pthread_mutex_lock( &sc->lock );
    while( 0 == sc->queue.count &&
        (sc->num_tasks > 0 || !sc->terminate) ) {
#ifdef MONITORING_ENABLE
      /* TODO only on flag */
      MonfileDebug(sc, "wid %d waiting (queue: %u, tasks: %u, term: %d)\n",
          sc->wid, sc->queue.count, sc->num_tasks, sc->terminate);
#endif
      pthread_cond_wait( &sc->cond, &sc->lock);
    }
    /* if queue is empty, t:=NULL */
    t = TaskqueuePopFront( &sc->queue);
    pthread_mutex_unlock( &sc->lock );

    if (t != NULL) {

      /* execute task */
      TaskCall(t, sc);
      
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

  /* MAIN SCHEDULER LOOP END */
#ifdef MONITORING_ENABLE
  MonfileDebug(sc, "worker %d exited (queue: %u, tasks: %u, term: %d)\n",
      sc->wid, sc->queue.count, sc->num_tasks, sc->terminate);
#endif
}


static void Wrapper( schedctx_t *sc)
{
  task_t *t;

  /* MAIN SCHEDULER LOOP */
  sc->loop=0;
  do {
    
    /* fetch the task: wait until it is ready */
    pthread_mutex_lock( &sc->lock );
    while( 0 == sc->queue.count ) {
      pthread_cond_wait( &sc->cond, &sc->lock);
    }
    /* if queue is empty, t:=NULL */
    t = TaskqueuePopFront( &sc->queue);
    assert(t != NULL);
    pthread_mutex_unlock( &sc->lock );

    /* execute task */
    TaskCall(t, sc);

    /* check state of task */
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
        //pthread_mutex_lock( &sc->lock );
        TaskqueuePushBack( &sc->queue, t );
        //pthread_mutex_unlock( &sc->lock );
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
  
}


/**
 * Startup thread for workers and wrappers
 */
static void *StartupThread( void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;
  
  /* Init libPCL */
  co_thread_init();

  if (sc->wid >= 0) {
    /* assign to cores */
    LpelThreadAssign( sc->wid);
    /* call worker function */
    Worker( sc);
  } else {
    /* assign to others */
    LpelThreadAssign( -1);
    /* call wrapper function */
    Wrapper( sc);
  }
  
  /* cleanup monitoring */
  MonfileClose( sc);
  
  /* clean up the scheduler data for the worker */
  pthread_mutex_unlock( &sc->lock );
  pthread_mutex_destroy( &sc->lock);
  pthread_cond_destroy( &sc->cond);
  
  /* free the scheduler context */
  if (sc->wid < 0) free( sc);

  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
}



/*****************************************************************************/
/*  Printing, for monitoring output                                          */
/*****************************************************************************/

void SchedPrintContext( schedctx_t *sc, FILE *file)
{
  if ( sc->wid < 0) {
    (void) fprintf(file, "loop %u ", sc->loop);
  } else {
    (void) fprintf(file, "wid %d loop %u ", sc->wid, sc->loop);
  }
}





