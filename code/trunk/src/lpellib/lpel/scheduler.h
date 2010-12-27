#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_


#include "lpel.h"


typedef struct schedctx schedctx_t;

void SchedInit( int num_workers);
void SchedCleanup( void);
schedctx_t *SchedCreate( int wid);
void SchedDestroy( schedctx_t *sc);

void SchedMakeReady( schedctx_t* sc, lpel_task_t *t);
lpel_task_t *SchedFetchReady( schedctx_t *sc);



#endif /* _SCHEDULER_H_ */
