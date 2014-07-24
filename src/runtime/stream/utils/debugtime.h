#ifndef _SNET_DEBUG_TIME_H_
#define _SNET_DEBUG_TIME_H_

#ifndef NULL
#include <stddef.h>
#endif

typedef struct timeval snet_time_t;

void SNetDebugTimeGetTime(snet_time_t *time);
long SNetDebugTimeGetMilliseconds(snet_time_t *time);
long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b);

/* Convert number of seconds as a double to a struct timeval. */
void SNetTimeFromDouble(snet_time_t *time, double real);

/* Convert a struct timeval to number of seconds as a double. */
double SNetTimeToDouble(snet_time_t *time);

/* Return wall-clock time. */
double SNetRealTime(void);

/* Return per-process consumed CPU time. */
double SNetProcessTime(void);

/* Return per-thread consumed CPU time. */
double SNetThreadTime(void);

/* Return current time in a string as HH:MM:SS.mmm. */
void SNetLocalTimeString(char *buf, size_t size);

#endif
