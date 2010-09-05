#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "task.h"
#include "monitoring.h"


typedef struct schedcfg schedcfg_t;


extern void SchedInit(int size, schedcfg_t *cfg);
extern void SchedCleanup(void);

extern void SchedAssignTask(int wid, task_t *t);
extern void SchedTask(int wid, monitoring_t *mon);


#endif /* _SCHEDULER_H_ */
