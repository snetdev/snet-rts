#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file distribution.c
 *
 * @brief Main programming interface to distributed S-Net.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 16.1.2009
 *
 *****************************************************************************/

#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

#include "debug.h"
#include "distribution.h"
#include "constants.h"
#include "routing.h"
#include "datastorage.h"
#include "omanager.h"
#include "imanager.h"
#include "message.h"
#include "id.h"

#ifdef SNET_TIME_COUNTERS
#include "debugtime.h"
#endif /* SNET_TIME_COUNTERS  */

/** <!--********************************************************************-->
 *
 * @name Ref
 *
 * <!--
 * int DistributionInit(int argc, char *argv[]) : Initializes disributed S-Net.
 * snet_tl_stream_t *DistributionStart(snet_startup_fun_t fun) : Constructs the S-Net.
 * void DistributionStop() : Stops distributed S-Net.
 * void DistributionDestroy() : Destroys distributed S-Net and frees resources.
 * snet_tl_stream_t *DistributionWaitForInput() : Waits until the node has input stream.
 * snet_tl_stream_t *DistributionWaitForOutput() : Waits until the node has output stream.
 * -->
 *
 *****************************************************************************/
/*@{*/

/** <!--********************************************************************-->
 *
 * @fn  int DistributionInit(int argc, char *argv[])
 *
 *   @brief  Initializes disributed S-Net.
 *
 *           Initiates all the services needed by distributed S-Net.
 * 
 *
 *   @param argc  As in main().
 *   @param argv  As in main().
 *
 *   @return      ID of the node.
 *
 ******************************************************************************/

#ifdef SNET_TIME_COUNTERS
static snet_time_t execution_start_time;
#endif /* SNET_TIME_COUNTERS */


int DistributionInit(int argc, char *argv[])
{
  int my_rank;
  int level;
  snet_tl_stream_t *stream;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level); 

#ifdef SNET_TIME_COUNTERS
  SNetDebugTimeGetTime(&execution_start_time);
#endif /* SNET_TIME_COUNTERS  */


  if(level != MPI_THREAD_MULTIPLE) {

    MPI_Finalize();

    SNetUtilDebugFatal("MPI thread support level \"MPI_THREAD_MULTIPLE\" required.");
    return -1;
  } 
 
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


  SNetIDServiceInit(my_rank);

  SNetMessageTypesInit();

  SNetRoutingInit(stream);

  SNetDataStorageInit();

  OManagerCreate();
  IManagerCreate();

  return my_rank;
}


/** <!--********************************************************************-->
 *
 * @fn  snet_tl_stream_t *DistributionStart(snet_startup_fun_t fun)
 *
 *   @brief  Construct distributed S-Net      
 *
 *   @param fun  Top-level S-Net function.
 * 
 *
 ******************************************************************************/

void DistributionStart(snet_startup_fun_t fun)
{
  int my_rank;
  snet_tl_stream_t *ret_stream, *input;
  snet_info_t *info;
  snet_fun_id_t fun_id;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if(my_rank == 0) {
    input = SNetTlCreateStream(BUFFER_SIZE);

    SNetDistFunFun2ID(fun, &fun_id);

    info = SNetInfoInit();

    SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, -1, &fun_id, my_rank));

    ret_stream = fun(input, info, my_rank);

    ret_stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), ret_stream);

    if(ret_stream != NULL) {
      SNetTlMarkObsolete(ret_stream);
    }

    SNetInfoDestroy(info);
  }

  return;
}


/** <!--********************************************************************-->
 *
 * @fn  void DistributionStop()
 *
 *   @brief  Stops the S-Net.
 *
 *           Blocks until the S-Net is stopped. Wakes threads that 
 *           are waiting for I/O.
 *
 ******************************************************************************/
void DistributionStop()
{
  int num_nodes;
  int i;
        
  MPI_Comm_size( MPI_COMM_WORLD, &num_nodes);

  for(i = 0; i < num_nodes; i++) {
    MPI_Send(&i, 0, MPI_INT, i, SNET_msg_terminate, MPI_COMM_WORLD);
  }
  
  SNetRoutingNotifyAll();
}

/** <!--********************************************************************-->
 *
 * @fn  int DistributionDestroy()
 *
 *   @brief  Frees resources hold by distributed the S-Net.
 *
 *
 ******************************************************************************/
void DistributionDestroy()
{
#ifdef SNET_TIME_COUNTERS
  snet_time_t execution_end_time;
  long mseconds; 
  long mseconds_in_box;
  long mseconds_in_transfers;
#endif /* SNET_TIME_COUNTERS  */

  SNetDataStorageDestroy();

  SNetIDServiceDestroy();

  SNetDistFunCleanup();

  SNetRoutingDestroy();

  SNetMessageTypesDestroy();

#ifdef SNET_TIME_COUNTERS
  SNetDebugTimeGetTime(&execution_end_time);

  mseconds = SNetDebugTimeDifferenceInMilliseconds(&execution_start_time, 
						   &execution_end_time);

  mseconds_in_box = SNetDebugTimeGetTimeCounter(SNET_TIME_COUNTER_BOX);
  mseconds_in_transfers = SNetDebugTimeGetTimeCounter(SNET_TIME_COUNTER_DATA_TRANSFER);


  SNetUtilDebugNotice("\nExecution time %ld milliseconds.\nTime spent in boxes: %ld milliseconds (%0.2lf%%)\nTime spent in data fetches: %ld milliseconds (%0.2lf%%).", 
		      mseconds, 
		      mseconds_in_box, ((double)mseconds_in_box * 100.0) / mseconds, 
		      mseconds_in_transfers, ((double)mseconds_in_transfers * 100.0)/ mseconds);
#endif /* SNET_TIME_COUNTERS  */

  MPI_Finalize();
}


/** <!--********************************************************************-->
 *
 * @fn  snet_tl_stream_t *DistributionWaitForInput()
 *
 *   @brief   Block until input stream of S-Net is in this node.   
 *
 *   @return  Input stream of the S-Net, or NULL if the waiting was interrupted. 
 *
 ******************************************************************************/

snet_tl_stream_t *DistributionWaitForInput()
{
  return SNetRoutingWaitForGlobalInput();
}


/** <!--********************************************************************-->
 *
 * @fn  snet_tl_stream_t *DistributionWaitForOutput()
 *
 *   @brief   Block until output stream of S-Net is in this node.   
 *
 *   @return  Output stream of the S-Net, or NULL if the waiting was interrupted. 
 *
 ******************************************************************************/

snet_tl_stream_t *DistributionWaitForOutput()
{
  return SNetRoutingWaitForGlobalOutput();
}

/*@}*/
#endif /* DISTRIBUTED_SNET */
