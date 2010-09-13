/**
 * Main LPEL module
 *
 */

#define _GNU_SOURCE

#include <malloc.h>
#include <assert.h>

#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <sys/types.h> /* pid_t */
#include <linux/unistd.h>
#include <sys/syscall.h> 

#include <pthread.h> /* worker threads are pthreads (linux has 1-1 mapping) */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "lpel.h"

/*!! link with -lcap */
#ifdef LPEL_USE_CAPABILITIES
#  include <sys/capability.h>
#endif

/* macro using syscall for gettid, as glibc doesn't provide a wrapper */
#define gettid() syscall( __NR_gettid )


#include "task.h"
#include "scheduler.h"

/* used (imported) modules of LPEL */
#include "monitoring.h"
#include "atomic.h"

/*
 * Global active entities count.
 */
atomic_t lpel_active_entities = ATOMIC_INIT(0);


struct lpelthread {
  pthread_t pthread;
};

/* Keep copy of the (checked) configuration provided at LpelInit() */
static lpelconfig_t config;

/* cpuset for others-threads */
static cpu_set_t cpuset_others;

static pthread_barrier_t bar_worker_init;

static pthread_t worker_launcher;


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
 * Get the number of workers
 */
int LpelNumWorkers(void)
{
  return config.num_workers;
}


/**
 * Poll the terminate condition for worker
 */
int LpelWorkerTerminate(void)
{
  return (atomic_read(&lpel_active_entities) <= 0);
}




/**
 * Startup the worker
 */
static void *WorkerStartup(void *arg)
{
  int res;
  pid_t tid;
  int wid = *((int *)arg);
  monitoring_t mon_info;

  /* Init libPCL */
  co_thread_init();

  /* initialise monitoring */
  MonitoringInit(&mon_info, wid, MONITORING_ALL);

  MonitoringDebug(&mon_info, "worker %d started\n", wid);

  /* get thread id */
  tid = gettid();

  /* set affinity to id % config->proc_workers */
  {
    int core = wid % config.proc_workers;
    cpu_set_t cpuset; 
    CPU_ZERO(&cpuset);
    CPU_SET( core, &cpuset);

    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
    if (res == 0) {
      MonitoringDebug(&mon_info, "worker %d assigned to core %d\n", wid, core);
    } else {
      /*TODO warning, check errno? */
    }
  }
  
  /* set worker as realtime thread */
  if ( (config.flags & LPEL_FLAG_REALTIME) != 0 ) {
    struct sched_param param;
    param.sched_priority = 1; /* lowest real-time, TODO other? */
    res = sched_setscheduler(tid, SCHED_FIFO, &param);
    if (res == 0) {
      MonitoringDebug(&mon_info, "worker %d set realtime priority\n", wid);
    } else {
      /*TODO warning, check errno? */
    }
  }
  
  /* barrier for all worker threads, assures that the workers
    are assigned to their CPUs */
  pthread_barrier_wait( &bar_worker_init );

  /**************************************************************************/
  /* Start the scheduler task */
  SchedTask( wid, &mon_info );
  /* scheduler task will return when there is no more work to do */
  /**************************************************************************/

  MonitoringDebug(&mon_info, "worker %d exiting\n", wid);

  /* cleanup monitoring */ 
  MonitoringCleanup(&mon_info);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  pthread_exit(NULL);
}


/**
 * A separate thread that launches the worker threads
 */
static void *WorkerLauncher(void *arg)
{
  int i, res;
  int nw = config.num_workers;
  pthread_t thids[nw];
  int wid[nw];

  /* Init libPCL */
  //co_thread_init();

  /* initialise barrier */
  res = pthread_barrier_init(
      &bar_worker_init,
      NULL,
      nw
      );
  assert(res == 0 );

  /* launch worker threads */
  for (i = 0; i < nw; i++) {
    wid[i] = i;
    res = pthread_create(&thids[i], NULL, WorkerStartup, &wid[i]);
    if (res != 0) {
      assert(0);
      /*TODO error*/
    }
  }

  /* join on finish */
  for (i = 0; i < nw; i++) {
    pthread_join(thids[i], NULL);
  }

  /* destroy barrier */
  res = pthread_barrier_destroy(&bar_worker_init);
  assert( res == 0 );

  /* Cleanup libPCL */
  //co_thread_cleanup();

  pthread_exit(NULL);
  return NULL;
}




/* test only for a single flag in LpelInit! */
#define LPEL_ICFG(f)   (( cfg->flags & (f) ) != 0 )

/**
 * Initialise the LPEL
 *
 * If FLAG_AUTO is set, values set for num_workers, proc_workers and
 * proc_others are ignored and set for default as follows:
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
void LpelInit(lpelconfig_t *cfg)
{
  int proc_avail;

  /* query the number of CPUs */
  proc_avail = sysconf(_SC_NPROCESSORS_ONLN);
  if (proc_avail == -1) {
    /*TODO _SC_NPROCESSORS_ONLN not available! */
  }

  if ( LPEL_ICFG( LPEL_FLAG_AUTO ) ) {

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

  /* create the cpu_set for other threads */
  {
    int  i;
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

  /* initialise scheduler */
  SchedInit(cfg->num_workers, NULL);

  /* store a copy of the corrected cfg */
  config = *cfg;

  /* Init libPCL */
  co_thread_init();
}

/**
 * Create and execute the worker threads
 * 
 * This function returns immediately.
 * A separate thread is created that creates the worker threads- joins on the worker threads
 */
void LpelRun(void)
{
  int res;
  /* create worker launcher */
  res = pthread_create(&worker_launcher, NULL, WorkerLauncher, NULL);
  assert( res == 0 );
}


/**
 * Cleans the LPEL up
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* in any case, wait until the workers are finished */
  pthread_join(worker_launcher, NULL);

  /* Cleanup scheduler */
  SchedCleanup();

  /* Cleanup libPCL */
  co_thread_cleanup();
}







lpelthread_t *LpelThreadCreate(
    void *(*start_routine)(void *), void *arg)
{
  lpelthread_t *lt = (lpelthread_t *) malloc( sizeof(lpelthread_t) );
  int res;

  /* create thread with default attributes */
  res = pthread_create(&lt->pthread, NULL, start_routine, arg);
  assert( res==0 );

  /* set the affinity mask */
  res = pthread_getaffinity_np( lt->pthread, sizeof(cpu_set_t), &cpuset_others);
  assert( res==0 );

  /* increase num of active entities in the LPEL system */
  atomic_inc(&lpel_active_entities);

  return lt;
}



void LpelThreadJoin( lpelthread_t *lt, void **joinarg)
{
  int res;
  res = pthread_join(lt->pthread, joinarg);
  assert( res==0 );

  free(lt);

  /* decrease number of active entities in the LPEL system */
  atomic_dec(&lpel_active_entities);
}


/**
 *
 * Has to be thread-safe.
 */
void LpelTaskToWorker(task_t *t)
{
  int to_worker;

  /* increase num of active entities in the LPEL system */
  atomic_inc(&lpel_active_entities);

  /*TODO placement module */
  to_worker = t->uid % config.num_workers;
  t->owner = to_worker;

  /* assign to appropriate sched context */
  SchedAssignTask( to_worker, t );
}

void LpelTaskRemove(task_t *t)
{
  int isfree = TaskDestroy(t);

  if (isfree != 0) {
    /* decrease number of active entities in the LPEL system */
    atomic_dec(&lpel_active_entities);
  }
}

