#ifdef DISTRIBUTED_SNET
#include <mpi.h>

#include "message.h"
#include "memfun.h"
#include "constants.h"

#define NUMBER_OF_MPI_TYPES 5
#define MAX_BLOCKS_PER_TYPE 5

static MPI_Datatype mpi_types[NUMBER_OF_MPI_TYPES];

void SNetMessageTypesInit()
{  
  int block_lengths[MAX_BLOCKS_PER_TYPE];
  MPI_Aint displacements[MAX_BLOCKS_PER_TYPE];
  MPI_Datatype types[MAX_BLOCKS_PER_TYPE];

  snet_msg_route_update_t route_update;
  snet_msg_route_index_t route_index;
  snet_msg_create_network_t create_network;

  /* SNET_msg_terminate */

  mpi_types[SNET_msg_terminate] = MPI_INT;

  /* SNET_msg_record */

  mpi_types[SNET_msg_record] = MPI_PACKED;

  /* SNET_msg_route_update */

  types[0] = SNET_ID_MPI_TYPE;  
  types[1] = MPI_INT;  
  types[2] = MPI_BYTE;
  
  block_lengths[0] = 1;
  block_lengths[1] = 1;
  block_lengths[2] = sizeof(stream_t *);
 
  MPI_Address(&route_update.op_id, displacements);
  MPI_Address(&route_update.node, displacements + 1);
  MPI_Address(&route_update.stream, displacements + 2);

  displacements[2] -= displacements[0];
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(3, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_update]);

  MPI_Type_commit(&mpi_types[SNET_msg_route_update]);

  /* SNET_msg_route_index */

  types[0] = SNET_ID_MPI_TYPE;
  types[1] = MPI_INT;
  types[2] = MPI_INT;
  
  block_lengths[0] = 1; 
  block_lengths[1] = 1; 
  block_lengths[2] = 1; 

  MPI_Address(&route_index.op_id, displacements);
  MPI_Address(&route_index.node, displacements + 1);
  MPI_Address(&route_index.index, displacements + 2);
  
  displacements[2] -= displacements[0];
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  
  MPI_Type_create_struct(3, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_route_index]);


  MPI_Type_commit(&mpi_types[SNET_msg_route_index]);

  /* SNET_msg_create_network */
  
  types[0] = SNET_ID_MPI_TYPE; 
  types[1] = MPI_INT;
  types[2] = MPI_INT;
  types[3] = MPI_INT;
  types[4] = MPI_CHAR;
  
  block_lengths[0] = 1;
  block_lengths[1] = 1;
  block_lengths[2] = 1;
  block_lengths[3] = 1;
  block_lengths[4] = 32;

  MPI_Address(&create_network.op_id, displacements);
  MPI_Address(&create_network.parent, displacements + 1);
  MPI_Address(&create_network.tag, displacements + 2);
  MPI_Address(&create_network.fun_id.id, displacements + 3);
  MPI_Address(&create_network.fun_id.lib[0], displacements + 4);
  
  displacements[4] -= displacements[0];
  displacements[3] -= displacements[0];
  displacements[2] -= displacements[0];
  displacements[1] -= displacements[0];
  displacements[0] = 0;

  MPI_Type_create_struct(5, block_lengths, displacements,
			 types, &mpi_types[SNET_msg_create_network]);

  MPI_Type_commit(&mpi_types[SNET_msg_create_network]);
}

MPI_Datatype SNetMessageGetMPIType(snet_message_t type)
{
  return mpi_types[type];
}
#include <stdio.h>
void SNetMessageTypesDestroy()
{
  MPI_Type_free(&mpi_types[SNET_msg_route_update]);
  MPI_Type_free(&mpi_types[SNET_msg_route_index]);
  MPI_Type_free(&mpi_types[SNET_msg_create_network]);
}

#endif /* DISTRIBUTED_SNET */
