#include <pthread.h>
#include "debugcounters.h"

static long counters[SNET_NUM_COUNTERS] = {0};
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDebugCountersIncreaseCounter(double value, int counter)
{
  if(counter < 0 || counter > SNET_NUM_COUNTERS) {
    return;
  }

  pthread_mutex_lock(&counter_mutex);

  counters[counter] += value;

  pthread_mutex_unlock(&counter_mutex);
}

double SNetDebugCountersGetCounter(int counter)
{
  long value;

  if(counter < 0 || counter > SNET_NUM_COUNTERS) {
    return 0;
  }

  pthread_mutex_lock(&counter_mutex);

  value = counters[counter];

  pthread_mutex_unlock(&counter_mutex);

  return value;
}
