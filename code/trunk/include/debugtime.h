#ifndef _SNET_DEBUG_TIME_H_
#define _SNET_DEBUG_TIME_H_

#ifdef DISTRIBUTED_SNET
#ifdef DEBUG_TIME_MPI
typedef double snet_time_t;
#else /* DEBUG_TIME_MPI */
typedef struct timeval snet_time_t;
#endif /* DEBUG_TIME_MPI */
#else /* DISTRIBUTED_SNET */
typedef struct timeval snet_time_t;
#endif /* DISTRIBUTED_SNET*/

void SNetDebugTimeGetTime(snet_time_t *time);
long SNetDebugTimeGetMilliseconds(snet_time_t *time);
long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b);


/* These can be used to store time used in certain task,
 * like box execution or data transfers (distributed S-Net) 
 * for instance.
 *
 */

#define SNET_TIME_COUNTER_BOX 0
#define SNET_TIME_COUNTER_DATA_TRANSFER 1
#define SNET_NUM_TIME_COUNTERS 2

void SNetDebugTimeIncreaseTimeCounter(long time, int counter);
long SNetDebugTimeGetTimeCounter(int counter);

#endif /* _SNET_DEBUG_TIME_H_ */
