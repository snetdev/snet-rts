#if defined(__linux__) && defined(HAVE_PTHREAD_SETAFFINITY_NP)
#define USE_CORE_AFFINITY 1
#endif

#include <stdarg.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <assert.h>

#include "threading.h"

/* local includes (only for pthread backend) */
#include "entity.h"
#include "monitorer.h"

#include "distribution.h"
#include "memfun.h"

static unsigned int entity_count = 0;
static pthread_mutex_t entity_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  entity_cond = PTHREAD_COND_INITIALIZER;
static pthread_key_t thread_self_key;


/* prototype for pthread thread function */
static void *EntityThread(void *arg);

static size_t StackSize(snet_entity_descr_t descr);

static void ThreadDestroy(snet_thread_t *self);

#ifdef USE_CORE_AFFINITY
typedef enum {
  DEFAULT,
  MASKMOD2,
  STRICTLYFIRST,
  ALLBUTFIRST,
  MAXCORES
} affinity_type_t;

static int SetThreadAffinity( pthread_t *pt, affinity_type_t at, ...);

#ifndef CPU_COUNT
/* Old pthread implementations do not have the CPU_COUNT macro. */
static inline int _my_cpu_count(cpu_set_t *set)
{
  int count = 0;

  for (int i = 0; i < CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, set)) count++;
  }

  return count;
}

#define CPU_COUNT(set)    _my_cpu_count(set)
#endif
#endif

unsigned long SNetThreadingGetId()
{
  /* returns the thread id */
  return (unsigned long) pthread_self(); /* returns a pointer on Mac */
}

int SNetThreadingInit(int argc, char **argv)
{
#ifdef USE_CORE_AFFINITY
  int i, num_cores;
  pthread_t selftid;
#endif
#ifdef USE_USER_EVENT_LOGGING
  char fname[32];
#endif
  /* initialize the entity counter to 0 */
  entity_count = 0;
  (void) argc; /* NOT USED */
  (void) argv; /* NOT USED */

  pthread_key_create(&thread_self_key, NULL);

#ifdef USE_CORE_AFFINITY
  for (i=0; i<argc; i++) {
    if(strcmp(argv[i], "-w") == 0 && i + 1 <= argc) {
      /* Number of cores */
      i = i + 1;
      num_cores = atoi(argv[i]);
      selftid = pthread_self();
      SetThreadAffinity( &selftid, MAXCORES,  num_cores);
    }
  }
#endif

#ifdef USE_USER_EVENT_LOGGING
  snprintf(fname, 31, "mon_n%02u_info.log", SNetDistribGetNodeId());
  SNetThreadingMonitoringInit(fname);
#endif

  return 0;
}


int SNetThreadingStop(void)
{
  /* Wait for the entities */
  pthread_mutex_lock( &entity_lock );
  while (entity_count > 0) {
    pthread_cond_wait( &entity_cond, &entity_lock );
  }
  pthread_mutex_unlock( &entity_lock );

  return 0;
}


int SNetThreadingCleanup(void)
{
  pthread_key_delete(thread_self_key);
  SNetThreadingMonitoringCleanup();
  return 0;
}


int SNetThreadingSpawn(snet_entity_t *ent)
{
  /*
  snet_entity_type_t type,
  snet_locvec_t *locvec,
  int location,
  const char *name,
  snet_entityfunc_t func,
  void *arg
  */
  int res;
  pthread_t p;
  pthread_attr_t attr;
  size_t stacksize;

  /* create snet_thread_t */
  snet_thread_t *thr = SNetMemAlloc(sizeof(snet_thread_t));
  pthread_mutex_init( &thr->lock, NULL );
  pthread_cond_init( &thr->pollcond, NULL );
  thr->wakeup_sd = NULL;
  thr->entity = ent;


  /* increment entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count += 1;
  pthread_mutex_unlock( &entity_lock );


  /* create pthread: */

  (void) pthread_attr_init( &attr);

  /* stacksize */
  stacksize = StackSize( SNetEntityDescr(ent) );

  if (stacksize > 0) {
    res = pthread_attr_setstacksize(&attr, stacksize);
    if (res != 0) {
      //error(1, res, "Cannot set stack size!");
      return 1;
    }
  }

  /* all threads are detached */
  res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (res != 0) {
    //error(1, res, "Cannot set detached!");
    return 1;
  }

  /* spawn off thread */
  res = pthread_create( &p, &attr, EntityThread, thr);
  if (res != 0) {
    //error(1, res, "Cannot create pthread!");
    return 1;
  }
  pthread_attr_destroy( &attr);


  /* core affinity */
#ifdef USE_CORE_AFFINITY
  if( SNetEntityDescr(ent)== ENTITY_other) {
    SetThreadAffinity( &p, STRICTLYFIRST);
  } else {
    SetThreadAffinity( &p, DEFAULT);
  }
#endif /* USE_CORE_AFFINITY */


  return 0;
}


#ifdef USE_USER_EVENT_LOGGING
void SNetThreadingEventSignal(snet_entity_t *ent, snet_moninfo_t *moninfo)
{
  SNetThreadingMonitoringAppend( moninfo, SNetEntityStr(ent) );
}
#endif

void SNetThreadingYield(void)
{
#ifdef HAVE_PTHREAD_YIELD_NP
  (void) pthread_yield_np();
#else
#ifdef HAVE_PTHREAD_YIELD
  (void) pthread_yield();
#endif
#endif
}


snet_thread_t *SNetThreadingSelf(void)
{
  return (snet_thread_t *) pthread_getspecific(thread_self_key);
}


/* void function, to support task migration for lpel_hrc */
void SNetThreadingCheckMigrate() {
}


/******************************************************************************
 * Private functions
 *****************************************************************************/

static void *EntityThread(void *arg)
{
  snet_thread_t *thr = (snet_thread_t *)arg;

  pthread_setspecific(thread_self_key, thr);

  SNetEntityCall(thr->entity);
  SNetEntityDestroy(thr->entity);

  /* call entity function */
  ThreadDestroy(thr);

  return NULL;
}

/**
 * Cleanup the thread
 */
static void ThreadDestroy(snet_thread_t *thr)
{
  /* cleanup */
  pthread_mutex_destroy( &thr->lock );
  pthread_cond_destroy(  &thr->pollcond );
  SNetMemFree(thr);


  /* decrement and signal entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count -= 1;
  if (entity_count == 0) {
    pthread_cond_signal( &entity_cond );
  }
  pthread_mutex_unlock( &entity_lock );
}

static size_t StackSize(snet_entity_descr_t descr)
{
  size_t stack_size;

  switch(descr) {
    case ENTITY_parallel:
    case ENTITY_star:
    case ENTITY_split:
    case ENTITY_fbcoll:
    case ENTITY_fbdisp:
    case ENTITY_fbbuf:
    case ENTITY_fbnond:
    case ENTITY_sync:
    case ENTITY_filter:
    case ENTITY_nameshift:
    case ENTITY_collect:
      stack_size = 256*1024; /* HIGHLY EXPERIMENTAL! */
      break;
    case ENTITY_box:
    case ENTITY_other:
      /* return 0 (caller will use default stack size) */
      stack_size = 0;
      break;
    default:
      /* we do not want an unhandled case here */
      assert(0);
  }

 return( stack_size);
}

#ifdef USE_CORE_AFFINITY
static int SetThreadAffinity( pthread_t *pt, affinity_type_t at, ...)
{
  int i, res, numcpus;
  cpu_set_t cpuset;

  res = pthread_getaffinity_np( *pt, sizeof(cpu_set_t), &cpuset);
  if (res != 0) {
    return 1;
  }

  numcpus = CPU_COUNT(&cpuset);
 
  switch(at) {
    case MASKMOD2:
      CPU_ZERO(&cpuset);
      for( i=0; i<numcpus; i+=2) {
        CPU_SET( i, &cpuset);
      }
      break;
    case STRICTLYFIRST:
      if( numcpus > 1) {
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
      }
      break;
    case ALLBUTFIRST:
      if( numcpus > 1) {
        CPU_CLR(0, &cpuset);
      }
      break;
    case MAXCORES: {
        int j, num_cores;
        va_list args;
        va_start( args, at);
        num_cores = va_arg( args, int);
        va_end( args);
        for( j=num_cores; j<numcpus; j++) {
          CPU_CLR( j, &cpuset);
        }
      }
      break;
    default:
      break;
  }

  /* set the affinity mask */
  res = pthread_setaffinity_np( *pt, sizeof(cpu_set_t), &cpuset);
  if( res != 0) {
    return 1;
  }

  return 0;
}
#endif

