
#ifndef _LPEL_H_
#define _LPEL_H_

#define LPEL_USE_CAPABILITIES

#include "task.h"

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
} lpelconfig_t;

#define LPEL_FLAG_AUTO     (1<<0)
#define LPEL_FLAG_REALTIME (1<<1)


typedef struct lpelthread lpelthread_t;

extern void LpelInit(lpelconfig_t *cfg);
extern void LpelRun(void);
extern void LpelCleanup(void);


extern int LpelNumWorkers(void);
extern int LpelWorkerTerminate(void);

extern lpelthread_t *LpelThreadCreate(
  void *(*start_routine)(void *), void *arg);

extern void LpelThreadJoin( lpelthread_t *lt, void **joinarg);

extern void LpelTaskToWorker(task_t *t);
extern void LpelTaskRemove(task_t *t);

#endif /* _LPEL_H_ */
