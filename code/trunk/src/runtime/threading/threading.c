/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "memfun.h"
#include "snetentities.h"
#include "debug.h"
#include "threading.h"

#ifdef DBG_RT_TRACE_THREAD_CREATE 
#include <sys/time.h>
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
    case ENTITY_collect_nondet:
    case ENTITY_collect_det:
      stack_size = 256*1024; /* HGHILY EXPERIMENTAL! */
      break;
    case ENTITY_box:
    case ENTITY_none:
    default:
      /* return 0 (caller will use default stack size) */
      stack_size = 0;
      break;
  }

 return( stack_size);   
}


extern void SNetThreadCreate( void *(*fun)(void*),
                              void *fun_args,
                  snet_entity_id_t id) 
{
  pthread_t thread;
  pthread_attr_t attr;
  size_t stack_size;
  int res;
#ifdef DBG_RT_TRACE_THREAD_CREATE 
  struct timeval t;
  #endif

  SNetUtilDebugNotice("Creating thread1");
  res = pthread_attr_init( &attr);

  SNetUtilDebugNotice("Creating thread2");

  stack_size = ThreadStackSize( id);

  SNetUtilDebugNotice("Creating thread3");
  if( stack_size > 0) {
  SNetUtilDebugNotice("Creating thread3.5b");
    pthread_attr_setstacksize( &attr, stack_size);
  SNetUtilDebugNotice("Creating thread3,5a");
  }
  
  SNetUtilDebugNotice("Creating thread4");
  pthread_attr_getstacksize( &attr, &stack_size);
  SNetUtilDebugNotice("Creating thread5");
  res = pthread_create( &thread, &attr, fun, fun_args);

  SNetUtilDebugNotice("Creating thread6");
  pthread_attr_destroy( &attr);
  SNetUtilDebugNotice("Thread created");
#ifdef DBG_RT_TRACE_THREAD_CREATE 
  gettimeofday( &t, NULL);
  SNetLockThreadMutex();
  SNetIncThreadCount();
  fprintf( stderr, 
           "[DBG::RT::Threads] Thread #%3d created (stack:%5dkb)         %lf\n", 
           SNetGetThreadCount(), stack_size/1024, t.tv_sec + t.tv_usec /1000000.0);
  SNetUnlockThreadMutex();
#endif

  if( res != 0) {
    fprintf( stderr, "\n\n  ** Fatal Error ** : Failed to create new "
                     "thread [error %d].\n\n", 
             res);
    fflush( stdout);
    exit( 1);
  }
  else {
    SNetUtilDebugNotice("Detaching thread");
    ThreadDetach( &thread);
    SNetUtilDebugNotice("Thread detached");
  }
}
