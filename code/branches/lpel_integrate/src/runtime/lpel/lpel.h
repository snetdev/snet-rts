
#ifndef _LPEL_H_
#define _LPEL_H_

#define LPEL_USE_CAPABILITIES


#include <pthread.h>

#include "bool.h"

#include "monitoring.h"

/**
 * Specification for configuration:
 *
 * proc_workers is the number of processors used for workers.
 * num_workers must be a multiple of proc_workers.
 * proc_others is the number of processors assigned to other than
 *   worker threads.
 * flags:
 *   AUTO - use default setting for num_workers, proc_workers, proc_others
 *   REALTIME - set realtime priority for workers, will succeed only if
 *              there is a 1:1 mapping of workers to procs,
 *              proc_others > 0 and the process has needed privileges.
 */
typedef struct {
  int num_workers;
  int proc_workers;
  int proc_others;
  int flags;
  int node;
} lpelconfig_t;

#define LPEL_FLAG_AUTO     (1<<0)
#define LPEL_FLAG_REALTIME (1<<1)


#define LPEL_THREADNAME_MAXLEN 20

typedef struct lpelthread lpelthread_t;

struct lpelthread {
  pthread_t pthread;
  bool detached;
  void (*func)(struct lpelthread *, void *);
  void *arg;
  int node;
  char name[LPEL_THREADNAME_MAXLEN+1];
#ifdef MONITORING_ENABLE
  monitoring_t mon;
#endif
};


extern void LpelInit(lpelconfig_t *cfg);
extern void LpelCleanup(void);


extern int LpelNumWorkers(void);

extern lpelthread_t *LpelThreadCreate( void (*func)(lpelthread_t *, void *),
    void *arg, bool detached, char *name);

extern void LpelThreadJoin( lpelthread_t *env);
extern void LpelThreadAssign( lpelthread_t *env, int core);


#endif /* _LPEL_H_ */
