
#include <stdlib.h>
#include <assert.h>

#include "scheduler.h"


#include "taskqueue.h"
#include "task.h"


struct schedctx {
  taskqueue_t queue[SCHED_NUM_PRIO];
};


schedctx_t *SchedCreate( int wid)
{
  int i;
  schedctx_t *sc = (schedctx_t *) malloc( sizeof(schedctx_t));
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    TaskqueueInit( &sc->queue[i]);
  }
  return sc;
}


void SchedDestroy( schedctx_t *sc)
{
  int i;
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    assert( sc->queue[i].count == 0);
  }
  free( sc);
}



void SchedMakeReady( schedctx_t* sc, lpel_task_t *t)
{
  int prio = t->sched_info.prio;

  if (prio < 0) prio = 0;
  if (prio >= SCHED_NUM_PRIO) prio = SCHED_NUM_PRIO-1;
#ifdef SCHED_LIFO
  TaskqueuePushBack( &sc->queue[prio], t);
#else
  TaskqueuePushBack( &sc->queue[prio], t);
#endif

}


lpel_task_t *SchedFetchReady( schedctx_t *sc)
{
  lpel_task_t *t = NULL;
  int i;
  for (i=SCHED_NUM_PRIO-1; i>=0; i--) {
    if (sc->queue[i].count > 0) {
      t = TaskqueuePopFront( &sc->queue[i]);
      break;
    }
  }

  return t;
}

