#ifndef _SNET_MESSAGE_H_
#define _SNET_MESSAGE_H_

#include <mpi.h>

#include "record.h"
#include "stream_layer.h"
#include "fun.h"

/* Message types. 
 *
 * The type is also used as MPI tag.
 *
 */

typedef enum {
  SNET_msg_terminate,
  SNET_msg_record,
  SNET_msg_route_update,   /* This message is to be used only inside a node!*/
  SNET_msg_route_index,
  SNET_msg_create_network
} snet_message_t;


/* SNET_msg_terminate:
 *
 * Only a MPI tag is sent, no real message 
 */


/* SNET_msg_record:
 *
 * Records are sent as MPI_PACKED 
 */

/* SNET_msg_route_update,
 *
 * This message is for internal use only! 
 */

typedef struct snet_msg_route_update {
  int op_id;
  int node;
  snet_tl_stream_t *stream;
} snet_msg_route_update_t;

/* SNET_msg_route_index:
 *
 */

typedef struct snet_msg_route_index {
  int op_id;
  int node;
  int index;
} snet_msg_route_index_t;

/* SNET_msg_create_network:
 *
 */

typedef struct snet_msg_create_network {
  int op_id;
  int starter;
  int parent;
  int tag;
  snet_fun_id_t fun_id;
} snet_msg_create_network_t;


/* TODO: */
typedef struct snet_msg_route_redirect {

} snet_msg_route_redirect_t;


/* TODO: */
typedef struct snet_msg_route_concatenate {
  int index;
  snet_tl_stream_t *stream;
  snet_record_t *rec;
} snet_msg_route_concatenate_t;


/* TODO: */
typedef struct {
  char lib[32];
} snet_msg_network_fetch_request_t;

/* TODO: */
typedef char * snet_msg_network_fetch_reply_t;


void SNetMessageTypesInit();

MPI_Datatype SNetMessageGetMPIType(snet_message_t type);

void SNetMessageTypesDestroy();


#endif /* _SNET_MESSAGE_H_ */
