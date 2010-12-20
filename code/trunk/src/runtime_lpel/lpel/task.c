
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
task_t *TaskCreate( taskfunc_t func, void *inarg, taskattr_t *attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );

  t->uid = fetch_and_inc(&taskseq);
  t->state = TASK_READY;
  t->prev = t->next = NULL;
  
  /* copy task attributes */
  t->attr = *attr;
  /* fix attributes */
  if (t->attr.stacksize <= 0) {
    t->attr.stacksize = TASK_ATTR_STACKSIZE_DEFAULT;
  }

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);
  
  t->sched_context = NULL;
  
  if (t->attr.flags & TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP(&t->times.creat);
  }
  t->cnt_dispatch = 0;

  /* init streamset to write */
  t->dirty_list = (stream_desc_t *)-1;
  

  t->code = func;
  t->inarg = inarg;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create( TaskStartup, (void *)t, NULL, t->attr.stacksize);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }
  pthread_mutex_init( &t->lock, NULL);
 
  return t;
}


/**
 * Destroy a task
 */
void TaskDestroy(task_t *t)
{
  pthread_mutex_destroy( &t->lock);
  atomic_destroy( &t->poll_token);
  /* delete the coroutine */
  co_delete(t->ctx);
  /* free the TCB itself*/
  free(t);
}



/**
 * Call a task (context switch to a task)
 *
 * @pre t->state == TASK_READY
 */
void TaskCall( task_t *t, schedctx_t *sc)
{
  /* aqiure TCB lock */
  pthread_mutex_lock( &t->lock);

  assert( t->state != TASK_RUNNING);
  
  t->sched_context = sc;
  t->cnt_dispatch++;
  t->state = TASK_RUNNING;
      
  if (t->attr.flags & TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP(&t->times.start);
  }

  /*
   * CAUTION: A coroutine must not run simultaneously in more than one thread!
   *          Therefore the whole call is protected by a lock.
   */
  /* CONTEXT SWITCH */
  co_call( t->ctx);

  /* task returns in every case in a different state */
  assert( t->state != TASK_RUNNING);

  /* output accounting info */
#ifdef MONITORING_ENABLE
  if (t->attr.flags & TASK_ATTR_MONITOR_OUTPUT) {
    TIMESTAMP( &t->times.stop);
    SchedTaskPrint( t);
  }
#endif
  
  /* unlock TCB */
  pthread_mutex_unlock( &t->lock);
}


/**
 * Block a task
 */
void TaskBlock( task_t *ct, int wait_on)
{
  ct->state = TASK_BLOCKED;
  ct->wait_on = wait_on;
  /* context switch */
  co_resume();
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskExit( task_t *ct)
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
void TaskYield( task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  ct->state = TASK_READY;
  /* context switch */
  co_resume();
}

/****************************************************************************/
/*  Printing, for monitoring output                                         */
/****************************************************************************/


#define FLAGS_TEST(vec,f)   (( (vec) & (f) ) == (f) )

void TaskPrint( task_t *t, FILE *file)
{
  /* early exit */
  if ( !FLAGS_TEST( t->attr.flags, TASK_ATTR_MONITOR_OUTPUT) ) return;

  /* print general info: name, disp.cnt, state */
  fprintf( file,
      "tid %lu disp %lu st %c%c ",
      t->uid, t->cnt_dispatch, t->state,
      (t->state==TASK_BLOCKED)? t->wait_on : ' '
      );

  /* print times */
  if ( FLAGS_TEST( t->attr.flags, TASK_ATTR_COLLECT_TIMES) ) {
    timing_t diff;
    if ( t->state == TASK_ZOMBIE) {
      fprintf( file, "creat ");
      TimingPrint( &t->times.creat, file);
    }
    TimingDiff( &diff, &t->times.start, &t->times.stop);
    fprintf( file, "et ");
    TimingPrint( &diff , file);
  }

  /* print stream info */
  if ( FLAGS_TEST( t->attr.flags, TASK_ATTR_COLLECT_STREAMS) ) {
    StreamPrintDirty( t, file);
  }
}




/****************************************************************************/
/* Private functions                                                        */
/****************************************************************************/

/**
 * Startup function for user specified task,
 * calls task function with proper signature
 *
 * @param data    the previously allocated task_t TCB
 */
static void TaskStartup(void *data)
{
  task_t *t = (task_t *)data;
  taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);
  /* if task function returns, exit properly */
  TaskExit(t);
}


