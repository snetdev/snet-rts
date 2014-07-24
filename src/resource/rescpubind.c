#if HAVE_SCHED_SETAFFINITY
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>

#elif HAVE_PTHREAD_SETAFFINITY_NP
#include <pthread.h>
#endif

#include <assert.h>
#include "resdefs.h"
#include "restopo.h"

#if HAVE_SCHED_SETAFFINITY
pid_t res_gettid(void)
{
  pid_t tid = (pid_t) syscall(SYS_gettid);
  return tid;
}
#endif

int res_bind_physical_proc(int physical_proc)
{
#if HAVE_SCHED_SETAFFINITY
  int res;
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(physical_proc, &set);
  res = sched_setaffinity(res_gettid(), sizeof(set), &set);
  if (res) res_perror("sched_setaffinity");
  return res;

#elif HAVE_PTHREAD_SETAFFINITY_NP
  int res;
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(physical_proc, &set);
  res = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
  if (res) res_perror("pthread_setaffinity_np");
  return res;

#elif __APPLE__ || __MACH__
  /* No thread-to-cpu binding available. */
  return 0;
#else
  #error There is no configured way to bind a thread to a processor.
#endif

}

int res_bind_logical_proc(int logical_proc)
{
  int res = 0;
  host_t* host = res_local_host();

  if (logical_proc >= 0 && logical_proc < host->nprocs) {
    proc_t* proc = host->procs[logical_proc];
    assert(logical_proc == proc->logical);
    res = res_bind_physical_proc(proc->physical);
  } else {
    res_error("[%s]: Invalid logical proc %d.\n", __func__, logical_proc);
  }

  return res;
}

