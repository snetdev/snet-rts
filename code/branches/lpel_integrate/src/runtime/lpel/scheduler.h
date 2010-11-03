#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "lpel.h"

struct task;

typedef struct schedcfg schedcfg_t;

typedef struct schedctx schedctx_t;


extern void SchedInit(int size, schedcfg_t *cfg);
extern void SchedCleanup(void);

struct lpelthread;

extern void SchedAssignTask( struct task *t, int wid);
extern void SchedWrapper( struct lpelthread *env, void *arg);
extern void SchedWakeup( struct task *by, struct task *whom);
extern void SchedTerminate( void);


#include <stdio.h>

extern void SchedPrintContext( schedctx_t *sc, FILE *file);

#endif /* _SCHEDULER_H_ */
