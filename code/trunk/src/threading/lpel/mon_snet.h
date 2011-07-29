#ifndef _MON_SNET_H_
#define _MON_SNET_H_


#define SNET_MON_TASK_TIMES   (1<<0)
#define SNET_MON_TASK_STREAMS (1<<1)
#define SNET_MON_TASK_USREVT  (1<<2)

/* IMPORTANT: following order of includes */
#include "threading.h"
#include "lpel.h"

struct mon_worker_t;
struct mon_task_t;
struct mon_stream_t;

void SNetThreadingMonInit(lpel_monitoring_cb_t *cb, int node);
void SNetThreadingMonCleanup(void);


struct mon_task_t *SNetThreadingMonTaskCreate(unsigned long tid,
    const char *name, unsigned long flags);


void SNetThreadingMonEvent(struct mon_task_t *mt,
    snet_threading_event_t evt);


#endif /* _MON_SNET_H_ */
