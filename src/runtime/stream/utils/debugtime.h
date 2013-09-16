#ifndef _SNET_DEBUG_TIME_H_
#define _SNET_DEBUG_TIME_H_

typedef struct timeval snet_time_t;

void SNetDebugTimeGetTime(snet_time_t *time);
long SNetDebugTimeGetMilliseconds(snet_time_t *time);
long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b);

/* Return wall-clock time. */
double SNetDebugRealTime(void);

/* Return per-process consumed CPU time. */
double SNetDebugProcessTime(void);

/* Return per-thread consumed CPU time. */
double SNetDebugThreadTime(void);

#endif
