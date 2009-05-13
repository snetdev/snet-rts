#ifndef _SNET_DEBUG_COUNTERS_H_
#define _SNET_DEBUG_COUNTERS_H_


#define SNET_COUNTER_TIME_BOX 0
#define SNET_COUNTER_TIME_DATA_TRANSFERS 1
#define SNET_COUNTER_NUM_DATA_OPERATIONS 2
#define SNET_COUNTER_NUM_DATA_FETCHES 3
#define SNET_COUNTER_PAYLOAD_DATA_FETCHES 4
#define SNET_NUM_COUNTERS 5

void SNetDebugCountersIncreaseCounter(double value, int counter);
double SNetDebugCountersGetCounter(int counter);

#endif /* _SNET_DEBUG_COUNTERS_H_ */
