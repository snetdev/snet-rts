#include "debugtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define MICRO   1e-6
#define NANO    1e-9

/* Convert a timeval to milliseconds as a double. */
#define TV2MS(t)        ((t)->tv_sec * 1000.0 + (t)->tv_usec / 1000.0)

/* Convert a timeval to seconds as a double. */
#define TV2SEC(t)       ((double) (t)->tv_sec + (double) (t)->tv_usec * MICRO)

/* Convert a struct timespec to seconds as a double. */
#define TS2SEC(t)       ((double) (t).tv_sec + (double) (t).tv_nsec * NANO)

/* Verify that a return value is equal to zero. */
#define CHECK(r)  do { if (r) { perror(__func__); exit(1); } } while (0)

void SNetDebugTimeGetTime(snet_time_t *time)
{
  gettimeofday(time, NULL);
}

long SNetDebugTimeGetMilliseconds(snet_time_t *time)
{
  return TV2MS(time);
}

long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b)
{
  return TV2MS(time_a) - TV2MS(time_b);
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
  return TV2SEC(time);
}

/* Return wall-clock time. */
double SNetRealTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_REALTIME, &ts);
  CHECK(r);
  return TS2SEC(ts);
}

#ifdef CLOCK_PROCESS_CPUTIME_ID
/* Return per-process consumed CPU time. */
double SNetProcessTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  CHECK(r);
  return TS2SEC(ts);
}
#endif

#ifdef CLOCK_THREAD_CPUTIME_ID
/* Return per-thread consumed CPU time. */
double SNetThreadTime(void)
{
  struct timespec ts = { 0, 0 };
  int r = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  CHECK(r);
  return TS2SEC(ts);
}
#endif

/* Return current time in a string as HH:MM:SS.mmm. */
void SNetLocalTimeString(char *buf, size_t size)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == 0) {
    struct tm *tm;
#if HAVE_LOCALTIME_R
    struct tm localbuf;
    tm = localtime_r(&tv.tv_sec, &localbuf);
#else
    tm = localtime(&tv.tv_sec);
#endif
    snprintf(buf, size, "%02d:%02d:%02d.%03d",
             tm->tm_hour, tm->tm_min, tm->tm_sec,
             (int) (tv.tv_usec / 1000));
  } else {
    /* Unlikely. */
    buf[0] = '\0';
  }
}

