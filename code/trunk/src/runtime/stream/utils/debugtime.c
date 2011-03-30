#include "debugtime.h"
#include <unistd.h>
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




