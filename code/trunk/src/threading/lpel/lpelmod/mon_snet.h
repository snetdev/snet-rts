#ifndef _MON_SNET_H_
#define _MON_SNET_H_


#define SNET_MON_TASK_TIMES   (1<<0)
#define SNET_MON_TASK_STREAMS (1<<1)
#define SNET_MON_USREVT  (1<<2)


#define MON_ENTNAME_MAXLEN  64
#define MON_FNAME_MAXLEN  63


/* IMPORTANT: following order of includes */
#include "threading.h"
#include <lpel.h>
#include <lpel/timing.h>

struct mon_worker_t;
struct mon_task_t;
struct mon_stream_t;



void SNetThreadingMonInit(lpel_monitoring_cb_t *cb, int node, int level);
void SNetThreadingMonCleanup(void);


struct mon_task_t *SNetThreadingMonTaskCreate(unsigned long tid, const char *name);


void SNetThreadingMonEvent(struct mon_task_t *mt,
    snet_moninfo_t *moninfo);

/* logging levels
 all level is activated by USE_LOGGGING (default: MAP and WORKER)
 box, timestamp, stream, entity are activated by USE_TASK_EVENT_LOGGING
 message is activated by USE_USER_EVENT_LOGGING
*/
#define MON_NUM_LEVEL 7
#define MON_MAP_LEVEL 1
#define MON_WORKER_LEVEL 2
#define MON_BOX_LEVEL 3
#define MON_TIMESTAMP_LEVEL 4
#define MON_STREAM_LEVEL 5
#define MON_ALL_ENTITY_LEVEL 6
#define MON_USREVT_LEVEL 7 //message event

#endif /* _MON_SNET_H_ */
