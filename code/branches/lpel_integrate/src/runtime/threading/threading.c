/* $Id$ */

#ifdef USE_CORE_AFFINITY
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "memfun.h"
#include "snetentities.h"
#include "debug.h"
#include "threading.h"

#include "lpel.h"
#include "task.h"
#include "scheduler.h"

#if 0

struct snet_thread {
  pthread_t *tid;
};
#define SNT_TID( t)  ((t)->tid)


#ifdef USE_CORE_AFFINITY
typedef enum { DEFAULT, MASKMOD2, STRICTLYFIRST, ALLBUTFIRST} taffy_t;

static void SetThreadAffinity( pthread_t *thread, taffy_t mode)
{
  int i, rv, numcores;
  cpu_set_t cpuset;

  int setsize;

  rv = sched_getaffinity( getpid(), sizeof( cpu_set_t), &cpuset);
  numcores = CPU_COUNT_S( sizeof( cpu_set_t), &cpuset);
  setsize = CPU_ALLOC_SIZE( numcores);

  if( rv != 0) {
    SNetUtilDebugNotice("Could not get thread affinity [ERR: %d].", rv);
  } 
  
  switch( mode) {
    case MASKMOD2: 
      CPU_ZERO_S( setsize, &cpuset);
      for( i=0; i<numcores; i+=2) {
        CPU_SET_S( i, setsize, &cpuset);
      }
      break;
    case STRICTLYFIRST:
      CPU_ZERO_S( setsize, &cpuset);
      CPU_SET_S( 0, setsize, &cpuset);
      break;
    case ALLBUTFIRST: 
      CPU_ZERO_S( setsize, &cpuset);
      for( i=1; i<numcores; i++) {
        CPU_SET_S( i, setsize, &cpuset);
      }
      break;
    default:
      break;
  } 
  rv = pthread_setaffinity_np( *thread, setsize, &cpuset);
  if( rv != 0) {
    SNetUtilDebugNotice("Could not set thread affinity [ERR: %d].", rv);
  }
  else {
  rv = pthread_getaffinity_np( *thread, sizeof( cpu_set_t), &cpuset);
  numcores = CPU_COUNT_S( sizeof( cpu_set_t), &cpuset);
  }
}
#endif


static void ThreadDetach( pthread_t *thread)
{
  int res;

  res = pthread_detach( *thread);

  if( res != 0) {
    fprintf( stderr, "\n\n  ** Fatal Error ** : Failed to detach "
                     "thread [error %d].\n\n", 
             res);
    fflush( stdout);
    exit( 1);
  }
}

/**
 * returns -1 if default stack size is to be used 
 */
static size_t ThreadStackSize( snet_entity_id_t id)
{
  size_t stack_size;

  switch( id) {
    case ENTITY_parallel_nondet:
    case ENTITY_parallel_det:
    case ENTITY_star_nondet:
    case ENTITY_star_det:
    case ENTITY_split_nondet:
    case ENTITY_split_det:
    case ENTITY_sync:
    case ENTITY_filter:
    case ENTITY_collect:
      stack_size = 256*1024; /* HGHILY EXPERIMENTAL! */
      break;
    case ENTITY_box:
    case ENTITY_dist:
    case ENTITY_none:
    default:
      /* return 0 (caller will use default stack size) */
      stack_size = 0;
      break;
  }

 return( stack_size);   
}


static snet_thread_t *ThreadCreate( void *(*fun)(void*),
                                    void *fun_args,
                                    snet_entity_id_t id,
                                    bool detach) 
{
  snet_thread_t *snt = NULL;
  pthread_t *thread;
  pthread_attr_t attr;
  size_t stack_size;
  int res;
#ifdef DBG_RT_TRACE_THREAD_CREATE 
  struct timeval t;
#endif

  thread = SNetMemAlloc( sizeof( pthread_t));
  res = pthread_attr_init( &attr);
  stack_size = ThreadStackSize( id);

  if( stack_size > 0) {
    pthread_attr_setstacksize( &attr, stack_size);
  }
  
  pthread_attr_getstacksize( &attr, &stack_size);
  res = pthread_create( thread, &attr, fun, fun_args);

#ifdef DBG_RT_TRACE_THREAD_CREATE 
  gettimeofday( &t, NULL);
  SNetGlobalLockThreadMutex();
  SNetGlobalIncThreadCount();
  fprintf( stderr, 
           "[DBG::RT::Threads] Thread #%3d created (stack:%5dkb)         %lf\n", 
           SNetGlobalGetThreadCount(), stack_size/1024, t.tv_sec + t.tv_usec /1000000.0);
  SNetGlobalUnlockThreadMutex();
#endif

  if( res != 0) {
    fprintf( stderr, "\n\n  ** Fatal Error ** : Failed to create new "
                     "thread [error %d].\n\n", 
             res);
    fflush( stdout);
    exit( 1);
  }
  else {
#ifdef USE_CORE_AFFINITY
#ifdef DISTRIBUTED_SNET
    if( id == ENTITY_dist) {
      SetThreadAffinity( thread, STRICTLYFIRST);
    } 
    else {
      SetThreadAffinity( thread, ALLBUTFIRST);
    }
#else
    SetThreadAffinity( thread, DEFAULT);
#endif /* DISTRIBUTED_SNET */
#endif /* USE_CORE_AFFINITY */

    pthread_attr_destroy( &attr);
    if( detach) {
      ThreadDetach( thread);
    } 
    else {
      snt = SNetMemAlloc( sizeof( snet_thread_t));
      SNT_TID( snt) = thread;
    }
  }

  return( snt);
}

void SNetThreadCreate( void *(*fun)(void*), 
                       void *fun_args,
                       snet_entity_id_t id)
{
  (void)ThreadCreate( fun, fun_args, id, true);
}

snet_thread_t *SNetThreadCreateNoDetach( void *(*fun)(void*), 
                                         void *fun_args,
                                         snet_entity_id_t id)
{
  return( ThreadCreate( fun, fun_args, id, false));
}

void SNetThreadJoin( snet_thread_t *t, void **ret)
{
  pthread_join( *SNT_TID( t), ret);
}

#endif




void SNetEntitySpawn( taskfunc_t fun, void *arg, snet_entity_id_t id)
{
  task_t *t;
  taskattr_t tattr = {0, 0};

  /* monitoring */
  if (id==ENTITY_box) {
    tattr.flags |= TASK_ATTR_MONITOR;
  }

  /* stacksize */
  if (id==ENTITY_box || id==ENTITY_none) {
    tattr.stacksize = 8*1024*1024; /* 8 MB */
  } else {
    tattr.stacksize = 256*1024; /* 256 kB */
  }

  /* waitany tasks */
  /*
  if ( id==ENTITY_collect ) {
    tattr.flags |= TASK_ATTR_WAITANY;
  }
  */

  /* create task */
  t = TaskCreate( fun, arg, &tattr);

  /* TODO better assignment! */
  {
    int wid = t->uid % LpelNumWorkers();
    SchedAssignTask( t, wid );
  }
}
