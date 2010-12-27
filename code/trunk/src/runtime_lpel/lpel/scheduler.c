
#include <stdlib.h>
#include <assert.h>

#include "scheduler.h"


#include "taskqueue.h"


struct schedctx {
  taskqueue_t queue;
};



void SchedInit( int num_workers)
{
  /* NOP */
}

void SchedCleanup( void)
{
  /* NOP */
}


schedctx_t *SchedCreate( int wid)
{
  schedctx_t *sc = (schedctx_t *) malloc( sizeof(schedctx_t));
  TaskqueueInit( &sc->queue);
  return sc;
}


void SchedDestroy( schedctx_t *sc)
{
  assert( sc->queue.count == 0);
  free( sc);
}



void SchedMakeReady( schedctx_t* sc, lpel_task_t *t)
{
  TaskqueueEnqueue( &sc->queue, t);
}


lpel_task_t *SchedFetchReady( schedctx_t *sc)
{
  return TaskqueueDequeue( &sc->queue);
}

