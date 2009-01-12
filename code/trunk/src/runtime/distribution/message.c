#ifdef DISTRIBUTED_SNET
#include <mpi.h>

#include "message.h"
#include "memfun.h"
#include "constants.h"

#define NUMBER_OF_MPI_TYPES 8
#define MAX_BLOCKS_PER_TYPE 3

static MPI_Datatype mpi_types[NUMBER_OF_MPI_TYPES];

void SNetMessageTypesInit()
{  
  int block_lengths[MAX_BLOCKS_PER_TYPE];
  MPI_Aint displacements[MAX_BLOCKS_PER_TYPE];
  MPI_Datatype types[MAX_BLOCKS_PER_TYPE];

  snet_msg_route_update_t route_update;
  snet_msg_create_network_t create_network;
  snet_msg_route_concatenate_t route_concatenate;
  snet_msg_route_redirect_t route_redirect;
  snet_msg_route_redirect_internal_t route_redirect_internal;

  /* SNET_msg_terminate */

  mpi_types[SNET_msg_terminate] = MPI_DATATYPE_NULL;

  /* SNET_msg_record */

  mpi_types[SNET_msg_record] = MPI_PACKED;

  /* SNET_msg_route_update */

  types[0] = MPI_INT;  
  types[1] = MPI_BYTE;
  
  block_lengths[0] = 2;
  block_lengths[1] = sizeof(snet_tl_stream_t *);
 
  MPI_Address(&route_update.op_id, displacements);
  MPI_Address(&route_update.stream, displacements + 1);
  
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_update]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_update]);

  /* SNET_msg_route_index */
  
  types[1] = MPI_INT;
  
  block_lengths[0] = 3; 
  
  displacements[0] = 0;
  
  MPI_Type_create_struct(1, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_index]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_index]);

  /* SNET_msg_create_network */
  
  types[0] = MPI_INT;
  types[1] = MPI_CHAR;
  
  block_lengths[0] = 5;
  block_lengths[1] = 32;
  
  MPI_Address(&create_network.op_id, displacements);
  MPI_Address(&create_network.fun_id.lib[0], displacements + 1);
  
  displacements[1] -= displacements[0];
  displacements[0] = 0;

  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_create_network]);

  MPI_Type_commit(&mpi_types[SNET_msg_create_network]);

  /* SNET_msg_route_concatenate */

  types[0] = MPI_INT;  
  types[1] = MPI_BYTE;
  
  block_lengths[0] = 1;
  block_lengths[1] = sizeof(snet_tl_stream_t *);
 
  MPI_Address(&route_concatenate.index, displacements);
  MPI_Address(&route_concatenate.stream, displacements + 1);
 
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_concatenate]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_concatenate]);

  /* SNET_msg_route_redirect */

  types[0] = MPI_INT;  
  types[1] = MPI_INT;
  
  block_lengths[0] = 1;
  block_lengths[1] = 1;
 
  MPI_Address(&route_redirect.node, displacements);
  MPI_Address(&route_redirect.index, displacements + 1);
  
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_redirect]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_redirect]);

  /* SNET_msg_route_redirect_internal */

  types[0] = MPI_INT;  
  types[1] = MPI_BYTE;
  
  block_lengths[0] = 1;
  block_lengths[1] = sizeof(snet_tl_stream_t *);
 
  MPI_Address(&route_redirect_internal.node, displacements);
  MPI_Address(&route_redirect_internal.stream, displacements + 1);
  
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_redirect_internal]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_redirect_internal]);

}

MPI_Datatype SNetMessageGetMPIType(snet_message_t type)
{
  return mpi_types[type];
}

void SNetMessageTypesDestroy()
{
  MPI_Type_free(&mpi_types[SNET_msg_route_update]);
  MPI_Type_free(&mpi_types[SNET_msg_route_index]);
  MPI_Type_free(&mpi_types[SNET_msg_create_network]);
  MPI_Type_free(&mpi_types[SNET_msg_route_concatenate]);
  MPI_Type_free(&mpi_types[SNET_msg_route_redirect]);
  MPI_Type_free(&mpi_types[SNET_msg_route_redirect_internal]);
}

#endif /* DISTRIBUTED_SNET */
