#include "debugtime.h"
#include <unistd.h>
#include <sys/time.h>

#ifdef DISTRIBUTED_SNET
#ifdef DEBUG_TIME_MPI
#include <mpi.h>
#define TIME_MPI
#endif /* DEBUG_TIME_MPI */
#endif /* DISTRIBUTED_SNET */

void SNetDebugTimeGetTime(snet_time_t *time)
{
#ifdef TIME_MPI
  *time = MPI_Wtime();
#else /* TIME_MPI */
  gettimeofday(time, NULL);
#endif /* TIME_MPI */
}

long SNetDebugTimeGetMilliseconds(snet_time_t *time)
{
#ifdef TIME_MPI 
  return *time * 1000.0;
#else /* TIME_MPI */
  return time->tv_sec * 1000.0 + time->tv_usec / 1000.0;
#endif /* TIME_MPI */  
}

long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b)
{
#ifdef TIME_MPI  
  return (*time_b - *time_a) * 1000;
#else /* TIME_MPI */
  return (time_b->tv_sec * 1000.0 + time_b->tv_usec / 1000.0) - (time_a->tv_sec * 1000.0 + time_a->tv_usec / 1000.0);
#endif /* TIME_MPI */    
}




