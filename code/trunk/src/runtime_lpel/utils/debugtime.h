#ifndef _SNET_DEBUG_TIME_H_
#define _SNET_DEBUG_TIME_H_

typedef struct timeval snet_time_t;

void SNetDebugTimeGetTime(snet_time_t *time);
long SNetDebugTimeGetMilliseconds(snet_time_t *time);
long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b);
#endif /* _SNET_DEBUG_TIME_H_ */
