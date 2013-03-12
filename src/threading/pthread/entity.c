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

#ifdef HAVE___THREAD
static TLSSPEC snet_thread_t *thread_self;
#else /* HAVE___THREAD */
static pthread_key_t thread_self_key;
#endif /* HAVE___THREAD */


/* prototype for pthread thread function */
static size_t StackSize(snet_entity_t ent);
static void *TaskWrapper(void *arg);

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

int SNetThreadingInit(int argc, char **argv)
{
#ifdef USE_CORE_AFFINITY
  int i, num_cores;
  pthread_t selftid;
#endif
  char fname[32];
  /* initialize the entity counter to 0 */
  entity_count = 0;
  (void) argc; /* NOT USED */
  (void) argv; /* NOT USED */

#ifndef HAVE___THREAD
  pthread_key_create(&thread_self_key, NULL);
#endif

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

  snprintf(fname, 31, "mon_n%02u_info.log", SNetDistribGetNodeId());
  SNetThreadingMonitoringInit(fname);

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
#ifndef HAVE___THREAD
  pthread_key_delete(thread_self_key);
#endif
  SNetThreadingMonitoringCleanup();
  return 0;
}


void SNetThreadingSpawn(snet_entity_t ent, int loc, snet_locvec_t *locvec,
                        const char *name, snet_taskfun_t func, void *arg)
{
  int res, namelen, size;
  pthread_t p;
  pthread_attr_t attr;

  /* if locvec is NULL then entity_other */
  assert(locvec != NULL || ent == ENTITY_other);

  /* create snet_thread_t */
  snet_thread_t *thr = SNetMemAlloc(sizeof(snet_thread_t));
  thr->fun = func;
  pthread_mutex_init( &thr->lock, NULL );
  pthread_cond_init( &thr->pollcond, NULL );
  thr->wakeup_sd = NULL;
  thr->inarg = arg;

  namelen = name ? strlen(name) : 0;
  size = (locvec ? SNetLocvecPrintSize(locvec) : 0) + namelen + 1;
  thr->name = SNetMemAlloc(size);
  strncpy(thr->name, name, namelen);
  if (locvec != NULL) SNetLocvecPrint(thr->name + namelen, locvec);
  thr->name[size-1] = '\0';

  /* increment entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count += 1;
  pthread_mutex_unlock( &entity_lock );


  /* create pthread: */

  (void) pthread_attr_init( &attr);

  size_t stacksize = StackSize(ent);
  if (stacksize > 0) {
    res = pthread_attr_setstacksize(&attr, stacksize);
    if (res != 0) perror("Cannot set stack size!");
  }

  /* all threads are detached */
  res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (res != 0) perror("Cannot set detached!");

  /* spawn off thread */
  res = pthread_create( &p, &attr, &TaskWrapper, thr);
  if (res != 0) perror("Cannot create pthread!");
  pthread_attr_destroy( &attr);


  /* core affinity */
#ifdef USE_CORE_AFFINITY
  if (ent == ENTITY_other) {
    SetThreadAffinity( &p, STRICTLYFIRST);
  } else {
    SetThreadAffinity( &p, DEFAULT);
  }
#endif /* USE_CORE_AFFINITY */
}

snet_thread_t *SNetThreadingSelf(void)
{
#ifdef HAVE___THREAD
  return thread_self;
#else
  return (snet_thread_t *) pthread_getspecific(thread_self_key);
#endif
}

void SNetThreadingRespawn(snet_taskfun_t fun)
{
  snet_thread_t *self = SNetThreadingSelf();
  if (fun) self->fun = fun;
  self->run = true;
}


void SNetThreadingEventSignal(snet_moninfo_t *moninfo)
{ SNetThreadingMonitoringAppend( moninfo, SNetThreadingGetName()); }

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

const char *SNetThreadingGetName(void) { return SNetThreadingSelf()->name; }

/******************************************************************************
 * Private functions
 *****************************************************************************/

static void *TaskWrapper(void *arg)
{
  snet_thread_t *self = arg;
#ifdef HAVE___THREAD
  thread_self = arg;
#else
  pthread_setspecific(thread_self_key, arg);
#endif

  do {
    self->run = false;
    self->fun(self->inarg);
  } while (self->run);

  /* cleanup */
  pthread_mutex_destroy( &self->lock );
  pthread_cond_destroy(  &self->pollcond );
  SNetMemFree(self->name);
  SNetMemFree(self);


  /* decrement and signal entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count -= 1;
  if (entity_count == 0) {
    pthread_cond_signal( &entity_cond );
  }
  pthread_mutex_unlock( &entity_lock );

  return NULL;
}

static size_t StackSize(snet_entity_t ent)
{
  size_t stack_size;

  switch(ent) {
    case ENTITY_parallel:
    case ENTITY_star:
    case ENTITY_split:
    case ENTITY_fbcoll:
    case ENTITY_fbdisp:
    case ENTITY_fbbuf:
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

