#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "lpel.h"

struct task;

typedef struct schedcfg schedcfg_t;

typedef struct schedctx schedctx_t;


void SchedInit(int size, schedcfg_t *cfg);
void SchedCleanup(void);
void SchedTerminate(void);

struct lpelthread;

void SchedAssignTask( struct task *t, int wid);
void SchedWrapper( struct task *t, char *name);
void SchedWakeup( struct task *by, struct task *whom);

void SchedTaskPrint( struct task *t);

#include <stdio.h>

void SchedPrintContext( schedctx_t *sc, FILE *file);

#endif /* _SCHEDULER_H_ */
