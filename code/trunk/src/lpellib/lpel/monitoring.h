#ifndef _MONITORING_H_
#define _MONITORING_H_


#include <stdio.h>
#include "lpel.h"

typedef struct monitoring_t monitoring_t;


monitoring_t *_LpelMonitoringCreate(int node, char *name);
void _LpelMonitoringDestroy( monitoring_t *mon);


void _LpelMonitoringDebug( monitoring_t *mon, const char *fmt, ...);

void _LpelMonitoringOutput( monitoring_t *mon, lpel_task_t *t);


#endif /* _MONITORING_H_ */
