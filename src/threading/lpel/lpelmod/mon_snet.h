#ifndef _MON_SNET_H_
#define _MON_SNET_H_


#define SNET_MON_TIME   (1<<0)
#define SNET_MON_TASK		  (1<<1)
#define SNET_MON_MESSAGE  	  (1<<2)
#define SNET_MON_WORKER  	  (1<<3)
#define SNET_MON_STREAM  	  (1<<4)
#define SNET_MON_MAP  	  (1<<5)
#define SNET_MON_LOAD	 (1<<6)

#define MON_ENTNAME_MAXLEN  64
#define MON_FNAME_MAXLEN  63


/* IMPORTANT: following order of includes */
#include "threading.h"
#include <lpel.h>
#include <lpel/timing.h>
#include <lpel/monitor.h>

struct mon_worker_t;
struct mon_task_t;
struct mon_stream_t;


int SNetGetMaxWait();
void SNetThreadingMonInit(lpel_monitoring_cb_t *cb, int num_workers, int node, int level);
void SNetThreadingMonCleanup(void);


struct mon_task_t *SNetThreadingMonTaskCreate(unsigned long tid, const char *name);


void SNetThreadingMonEvent(struct mon_task_t *mt,
    snet_moninfo_t *moninfo);

/* allocation flags
 *   r: roundrobin
 *   l: load-balancing
 *   lb: load-balancing for box, roundrobin for non-box
 */
#define ALLOC_RR_FLAG "r"
#define ALLOC_LB_FLAG "l"
#define ALLOC_LB_BOX_FLAG "lb"


#define ALLOC_DEFAULT_MODE 1
#define ALLOC_RR_MODE 1
#define ALLOC_LB_MODE 2
#define ALLOC_LB_BOX_MODE 3




/* logging flags
 - m: mapping
 - t: timestamp
 - W: worker events
 - T: task events
 - S: stream traces (only with task events)
 - M: message traces (only with task events)
 - A: all
*/
#define MON_MAP_FLAG 'm'
#define MON_TIME_FLAG 't'
#define MON_WORKER_FLAG 'W'
#define MON_TASK_FLAG 'T'
#define MON_STREAM_FLAG 'S'
#define MON_MESSAGE_FLAG 'M'
#define MON_ALL_FLAG 'A'
#define MON_LOAD_FLAG 'L'

/* special characters */
//#define END_LOG_ENTRY '\n'		// for separate each log entry by one line --> for testing
#define END_LOG_ENTRY 			'#'			// more efficient for file writing, just send when the buffer is full
#define END_STREAM_TRACE 		'|'
#define MESSAGE_TRACE_SEPARATOR ';'			// used to separate message traces
#define WORKER_START_EVENT 		'S'
#define WORKER_WAIT_EVENT 		'W'
#define WORKER_END_EVENT 		'E'

#define LOG_FORMAT_VERSION		"Log format version 2.2 (since 05/03/2012)"

#endif /* _MON_SNET_H_ */
