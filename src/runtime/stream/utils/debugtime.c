#include "debugtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
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

/* Convert number of seconds as a double to a struct timeval. */
void SNetTimeFromDouble(snet_time_t *time, double real)
{
  const double truncated = trunc(real);
  const double remainder = real - truncated;
  const double million = 1e6;
  time->tv_sec = lrint(truncated);
  time->tv_usec = lrint(trunc(million * remainder));
}

/* Convert a struct timeval to number of seconds as a double. */
double SNetTimeToDouble(snet_time_t *time)
{
  const double micro = 1e-6;
  return ((double) time->tv_sec + micro * (double) time->tv_usec);
}

/* Verify that the return value from clock_gettime is equal to zero. */
#define CHECK(r)  do { if (r) { perror(__func__); exit(1); } } while (0)

/* Convert a timespec with nanosecond precision to double. */
#define NANO                  1e-9
#define timespec_to_double(t) ((double)(t).tv_sec + NANO * (double)(t).tv_nsec)

/* Return wall-clock time. */
double SNetRealTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_REALTIME, &ts);
  CHECK(r);
  return timespec_to_double(ts);
}

#ifdef CLOCK_PROCESS_CPUTIME_ID
/* Return per-process consumed CPU time. */
double SNetProcessTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  CHECK(r);
  return timespec_to_double(ts);
}
#endif

#ifdef CLOCK_THREAD_CPUTIME_ID
/* Return per-thread consumed CPU time. */
double SNetThreadTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  CHECK(r);
  return timespec_to_double(ts);
}
#endif

