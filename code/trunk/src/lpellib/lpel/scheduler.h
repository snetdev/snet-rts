#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_


#include "lpel.h"

//#define SCHED_LIFO

typedef struct schedctx schedctx_t;

schedctx_t *SchedCreate( int wid);
void SchedDestroy( schedctx_t *sc);

int SchedMakeReady( schedctx_t* sc, lpel_task_t *t);
lpel_task_t *SchedFetchReady( schedctx_t *sc);



#endif /* _SCHEDULER_H_ */
