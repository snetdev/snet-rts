
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "scheduler.h"


#include "taskqueue.h"


struct schedctx {
  taskqueue_t queue;
  pthread_mutex_t lock;
};


schedctx_t *SchedCreate( int wid)
{
  schedctx_t *sc = (schedctx_t *) malloc( sizeof(schedctx_t));
  pthread_mutex_init( &sc->lock, NULL);
  TaskqueueInit( &sc->queue);
  return sc;
}


void SchedDestroy( schedctx_t *sc)
{
  assert( sc->queue.count == 0);
  pthread_mutex_destroy( &sc->lock);
  free( sc);
}



int SchedMakeReady( schedctx_t* sc, lpel_task_t *t)
{
  int was_empty;

  pthread_mutex_lock( &sc->lock);
  was_empty = (sc->queue.count == 0);
#ifdef SCHED_LIFO
  TaskqueuePushFront( &sc->queue, t);
#else
  TaskqueuePushBack( &sc->queue, t);
#endif
  pthread_mutex_unlock( &sc->lock);

  return was_empty;
}


lpel_task_t *SchedFetchReady( schedctx_t *sc)
{
  lpel_task_t *t;
  
  pthread_mutex_lock( &sc->lock);
  t = TaskqueuePopFront( &sc->queue);
  pthread_mutex_unlock( &sc->lock);

  return t;
}

