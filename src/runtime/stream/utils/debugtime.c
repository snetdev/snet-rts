#include "debugtime.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

void SNetDebugTimeGetTime(snet_time_t *time)
{
  gettimeofday(time, NULL);
}

long SNetDebugTimeGetMilliseconds(snet_time_t *time)
{
  return time->tv_sec * 1000.0 + time->tv_usec / 1000.0;
}

long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b)
{
  return (time_b->tv_sec * 1000.0 + time_b->tv_usec / 1000.0) - (time_a->tv_sec * 1000.0 + time_a->tv_usec / 1000.0);
}

#ifdef CLOCK_REALTIME
/* Return wall-clock time. */
double SNetDebugRealTime(void)
{
  struct timespec ts = { 0, 0 };
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
#endif

#ifdef CLOCK_PROCESS_CPUTIME_ID
/* Return per-process consumed CPU time. */
double SNetDebugProcessTime(void)
{
  struct timespec ts = { 0, 0 };
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
#endif

#ifdef CLOCK_THREAD_CPUTIME_ID
/* Return per-thread consumed CPU time. */
double SNetDebugThreadTime(void)
{
  struct timespec ts = { 0, 0 };
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
#endif



