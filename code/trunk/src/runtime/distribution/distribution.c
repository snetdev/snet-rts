#ifdef DISTRIBUTED_SNET
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

#include "distribution.h"
#include "constants.h"
#include "routing.h"
#include "datastorage.h"
#include "omanager.h"
#include "imanager.h"
#include "message.h"
#include "id.h"


snet_tl_stream_t *DistributionInit(int argc, char *argv[], snet_startup_fun_t fun)
{
  int my_rank;
  int level;
  snet_tl_stream_t *ret_stream, *stream, *input;
  snet_routing_info_t *info;
  snet_fun_id_t fun_id;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level); 
 
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  stream = SNetTlCreateUnboundedStream();

  SNetIDServiceInit(my_rank);

  SNetMessageTypesInit();

  SNetRoutingInit(stream);

  SNetDataStorageInit();

  if(my_rank == 0) {
    input = SNetTlCreateUnboundedStream();

    SNetDistFunFun2ID(fun, &fun_id);

    info = SNetRoutingInfoInit(SNetRoutingGetNewID(), my_rank, -1, &fun_id, my_rank);

    ret_stream = fun(input, info, my_rank);

    ret_stream = SNetRoutingInfoFinalize(info, ret_stream);

    SNetRoutingInfoDestroy(info);
  }

  OManagerCreate(stream);
  IManagerCreate(stream);

  return ret_stream;
}

int DistributionDestroy()
{
  int my_rank;
  int num_nodes;
  int i;

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  if(SNetRoutingGetGlobalOutput() != NULL) {
      
    MPI_Comm_size( MPI_COMM_WORLD, &num_nodes);

    for(i = 0; i < num_nodes; i++) {
      MPI_Send(&i, 0, MPI_INT, i, SNET_msg_terminate, MPI_COMM_WORLD); 
    }

    SNetRoutingNotifyAll();
  }

  SNetDataStorageDestroy();

  SNetIDServiceDestroy();

  SNetDistFunCleanup();

  SNetRoutingDestroy();

  SNetMessageTypesDestroy();

  MPI_Finalize();

  return 0;
}

snet_tl_stream_t *DistributionWaitForInput()
{
  return SNetRoutingWaitForGlobalInput();
}

snet_tl_stream_t *DistributionWaitForOutput()
{
  return SNetRoutingWaitForGlobalOutput();
}

#endif /* DISTRIBUTED_SNET */
