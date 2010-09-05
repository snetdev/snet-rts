#ifndef _MONITORING_H_
#define _MONITORING_H_

/**
 * Monitoring prints internals of a task
 */
#include "task.h"


#define MONITORING_TIMES        (1<<0)
#define MONITORING_STREAMINFO   (1<<1)

#define MONITORING_ALLTASKS     (1<<8)

#define MONITORING_NONE           ( 0)
#define MONITORING_ALL            (-1)

typedef struct {
  FILE *outfile;
  int   flags;
} monitoring_t;


extern void MonitoringInit(monitoring_t *mon, int worker_id, int flags);
extern void MonitoringCleanup(monitoring_t *mon);
extern void MonitoringPrint(monitoring_t *mon, task_t *t);
extern void MonitoringDebug(monitoring_t *mon, const char *fmt, ...);




#endif /* _MONITORING_H_ */
