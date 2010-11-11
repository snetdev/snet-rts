#ifndef _SNET_MESSAGE_H_
#define _SNET_MESSAGE_H_

#include <mpi.h>

#include "record.h"
#include "stream.h"
#include "fun.h"

/* Message types. 
 *
 * The type is also used as MPI tag.
 *
 */

typedef enum {
  /* Control messages */
  SNET_msg_terminate,
  SNET_msg_route_update,   /* This message is to be used only inside a node!*/
  SNET_msg_route_index,
  SNET_msg_create_network,
  /* NOTICE: all message types above this line may collide with stream indices
   *         and cannot be received by the thread of the input manager that 
   *         handles control messages! */
  SNET_msg_record, 
} snet_message_t;

#define SNET_MESSAGE_NUM_CONTROL_MESSAGES 4

/* SNET_msg_terminate:
 *
 * Only a MPI tag is sent, no real message 
 */

/* SNET_msg_route_update,
 *
 * This message is for internal use only! 
 */

typedef struct snet_msg_route_update {
  snet_id_t op_id;
  int node;
  stream_t *stream;
} snet_msg_route_update_t;

/* SNET_msg_route_index:
 *
 */

typedef struct snet_msg_route_index {
  snet_id_t op_id;
  int node;
  int index;
} snet_msg_route_index_t;

/* SNET_msg_create_network:
 *
 */

typedef struct snet_msg_create_network {
  snet_id_t op_id;
  int parent;
  int tag;
  snet_fun_id_t fun_id;
} snet_msg_create_network_t;


/* SNET_msg_record:
 *
 * Records are sent as MPI_PACKED 
 */

void SNetMessageTypesInit();

MPI_Datatype SNetMessageGetMPIType(snet_message_t type);

void SNetMessageTypesDestroy();


#endif /* _SNET_MESSAGE_H_ */
