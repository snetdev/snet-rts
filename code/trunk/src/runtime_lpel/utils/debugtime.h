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

#endif /* _SNET_DEBUG_TIME_H_ */
