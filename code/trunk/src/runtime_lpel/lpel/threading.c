/**
 * Main LPEL module
 *
 */

#define _GNU_SOURCE

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <sys/types.h> /* pid_t */
#include <linux/unistd.h>
#include <sys/syscall.h> 

#include <pthread.h> /* worker threads are OS threads */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "threading.h"

/*!! link with -lcap */
#ifdef LPEL_USE_CAPABILITIES
#  include <sys/capability.h>
#endif

/* macro using syscall for gettid, as glibc doesn't provide a wrapper */
#define gettid() syscall( __NR_gettid )


#include "task.h"
#include "worker.h"



/* Keep copy of the (checked) configuration provided at LpelInit() */
static lpel_config_t config;

/* cpuset for others-threads */
static cpu_set_t cpuset_others;
static int proc_avail = -1;


static void CleanupEnv( lpel_thread_t *env)
{
  //TODO
  free( env);
}


static int CanSetRealtime(void)
{
#ifdef LPEL_USE_CAPABILITIES
  cap_t caps;
  cap_flag_value_t cap;
  /* obtain caps of process */
  caps = cap_get_proc();
  if (caps == NULL) {
    /*TODO error msg*/
    return 0;
  }
  cap_get_flag(caps, CAP_SYS_NICE, CAP_EFFECTIVE, &cap);
  return (cap == CAP_SET);
#else
  return 0;
#endif
}




/**
 * Startup a lpel thread
 */
static void *ThreadStartup( void *arg)
{
  lpel_thread_t *env = (lpel_thread_t *)arg;

  _LpelThreadAssign(-1);

  /* Init libPCL */
  co_thread_init();

  /* call the function */
  env->func( env->arg);

  /* if detached, cleanup the env now,
     otherwise it will be done on join */
  if (env->detached) {
    CleanupEnv( env);
  }

  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
}




/* test only for a single flag in CheckConfig */
#define LPEL_ICFG(f)   (( cfg->flags & (f) ) != 0 )
static void CheckConfig( lpel_config_t *cfg)
{

  /* query the number of CPUs */
  proc_avail = sysconf(_SC_NPROCESSORS_ONLN);
  if (proc_avail == -1) {
    /*TODO _SC_NPROCESSORS_ONLN not available! */
  }

  if ( LPEL_ICFG( LPEL_FLAG_AUTO2 ) ) {
      /* as many workers as processors */
      cfg->num_workers = cfg->proc_workers = proc_avail;
      cfg->proc_others = 0;


  } else if ( LPEL_ICFG( LPEL_FLAG_AUTO ) ) {

    /* default values */
    if (proc_avail > 1) {
      /* multiprocessor */
      cfg->num_workers = cfg->proc_workers = (proc_avail-1);
      cfg->proc_others = 1;
    } else {
      /* uniprocessor */
      cfg->num_workers = cfg->proc_workers = 1;
      cfg->proc_others = 0;
    }
  } else {

    /* sanity checks */
    if ( cfg->num_workers <= 0 ||  cfg->proc_workers <= 0 ) {
      /* TODO error */
      assert(0);
    }
    if ( cfg->proc_others < 0 ) {
      /* TODO error */
      assert(0);
    }
    if ( (cfg->num_workers % cfg->proc_workers) != 0 ) {
      /* TODO error */
      assert(0);
    }
  }

  /* check realtime flag sanity */
  if ( LPEL_ICFG( LPEL_FLAG_REALTIME ) ) {
    /* check for validity */
    if (
        (proc_avail < cfg->proc_workers + cfg->proc_others) ||
        ( cfg->proc_others == 0 ) ||
        ( cfg->num_workers != cfg->proc_workers )
       ) {
      /*TODO warning */
      /* clear flag */
      cfg->flags &= ~(LPEL_FLAG_REALTIME);
    }
    /* check for privileges needed for setting realtime */
    if ( CanSetRealtime()==0 ) {
      /*TODO warning */
      /* clear flag */
      cfg->flags &= ~(LPEL_FLAG_REALTIME);
    }
  }
}


static void CreateCpusetOthers( lpel_config_t *cfg)
{
  int  i;
  /* create the cpu_set for other threads */
  CPU_ZERO( &cpuset_others );
  if (cfg->proc_others == 0) {
    /* distribute on the workers */
    for (i=0; i<cfg->proc_workers; i++) {
      CPU_SET(i, &cpuset_others);
    }
  } else {
    /* set to proc_others */
    for( i=cfg->proc_workers;
        i<cfg->proc_workers+cfg->proc_others;
        i++ ) {
      CPU_SET(i, &cpuset_others);
    }
  }
}



/**
 * Initialise the LPEL
 *
 * If FLAG_AUTO is set, values set for num_workers, proc_workers and
 * proc_others are ignored and set for default as follows:
 *
 * AUTO2: proc_workers = num_workers = #proc_avail
 *        proc_others = 0
 *
 * AUTO: if #proc_avail > 1:
 *         num_workers = proc_workers = #proc_avail - 1
 *         proc_others = 1
 *       else
 *         proc_workers = num_workers = 1,
 *         proc_others = 0
 *
 * otherwise, following sanity checks are performed:
 *
 *  num_workers, proc_workers > 0
 *  proc_others >= 0
 *  num_workers = i*proc_workers, i>0
 *
 * If the realtime flag is invalid or the process has not
 * the appropriate privileges, it is ignored and a warning
 * will be displayed
 *
 * REALTIME: only valid, if
 *       #proc_avail >= proc_workers + proc_others &&
 *       proc_others != 0 &&
 *       num_workers == proc_workers 
 *
 */
void LpelInit(lpel_config_t *cfg)
{
  workercfg_t worker_config;

  /* store a local copy of cfg */
  config = *cfg;
  
  /* check (and correct) the config */
  CheckConfig( &config);
  /* create the cpu affinity set for non-worker threads */
  CreateCpusetOthers( &config);

  /* Init libPCL */
  co_thread_init();
 
  worker_config.node = config.node;
  /* initialise workers */
  _LpelWorkerInit( config.num_workers, &worker_config);

}

/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* Cleanup scheduler */
  _LpelWorkerCleanup();

  /* Cleanup libPCL */
  co_thread_cleanup();
}




/**
 * @pre core in [0, procs_avail] or -1
 */
void _LpelThreadAssign( int core)
{
  int res;
  pid_t tid;

  /* get thread id */
  tid = gettid();

  if ( core == -1) {
    /* assign to others cpuset */
    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset_others);
    assert( res == 0);

  } else {
    /* assign to specified core */
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET( core % proc_avail, &cpuset);
    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
    assert( res == 0);

    /* make non-preemptible */
    if ( (config.flags & LPEL_FLAG_REALTIME) != 0 ) {
      struct sched_param param;
      param.sched_priority = 1; /* lowest real-time, TODO other? */
      res = sched_setscheduler(tid, SCHED_FIFO, &param);
      assert( res == 0);
    }
  }
}




/**
 * Aquire a thread from the LPEL
 */
lpel_thread_t *LpelThreadCreate( void (*func)(void *),
    void *arg, bool detached)
{
  int res;
  pthread_attr_t attr;

  lpel_thread_t *env = (lpel_thread_t *) malloc( sizeof( lpel_thread_t));

  env->func = func;
  env->arg = arg;
  env->detached = detached;


  /* create attributes for joinable/detached*/
  pthread_attr_init( &attr);
  pthread_attr_setdetachstate( &attr,
      (detached) ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE
      );
  
  res = pthread_create( &env->pthread, &attr, ThreadStartup, env);
  assert( res==0 );

  pthread_attr_destroy( &attr);

  return env;
}


/**
 * @pre  thread must not have been created detached
 * @post lt is freed and the reference is invalidated
 */
void LpelThreadJoin( lpel_thread_t *env)
{
  int res;
  assert( env->detached == false);
  res = pthread_join(env->pthread, NULL);
  assert( res==0 );

  /* cleanup */
  CleanupEnv( env);
}

/**
 * Get the number of workers
 */
int LpelNumWorkers(void)
{
  return config.num_workers;
}

