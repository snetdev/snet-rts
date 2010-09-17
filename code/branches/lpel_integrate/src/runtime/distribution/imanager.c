#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file imanager.c
 *
 * @brief    Manages incoming connection of a node of distributed S-Net
 *
 *           One input thread per connection is created.
 *           In addition there is one master thread to listen for control messages.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/

#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "imanager.h"
#include "routing.h"
#include "message.h"
#include "memfun.h"
#include "record.h"
#include "interface_functions.h"
#include "fun.h"
#include "id.h"
#include "queue.h"
#include "debug.h"

#include "lpel.h"
#include "stream.h"
#include "inport.h"

/** <!--********************************************************************-->
 *
 * @name OManager
 *
 * <!--
 * static void *IManagerInputThread( void *ptr) : Main loop of input thread.
 * static void IManagerCreateInput(stream_t *stream, int node, int index) : Create a new input thread.
 * static void *IManagerCreateNetwork(void *op)  : Create a new network.
 * static int IManagerMatchUpdateToIndex(snet_queue_t *queue, snet_id_t op, int node) : Get buffered index message that matched the update message.
 * static void IOManagerPutIndexToBuffer(snet_queue_t *queue, snet_id_t op, int node, int index) : Buffer unused index message.
 * static stream_t *IManagerMatchIndexToUpdate(snet_queue_t *queue, snet_id_t op, int node) : Get buffered update message that matches the index message.
 * static void IOManagerPutUpdateToBuffer(snet_queue_t *queue, snet_id_t op, int node, stream_t *stream) : Buffer unused update message.
 * static void *IManagerMasterThread( void *ptr) : Main loop for master thread.
 * void SNetIManagerCreate() : Create input manager.
 * void SNetIManagerDestroy() : Destroy input manager.
 * -->
 *
 *****************************************************************************/
/*@{*/

#define ORIGINAL_MAX_MSG_SIZE 256  
#define INVALID_INDEX -1

/* Control message types */
#define NUM_MESSAGE_TYPES 4

#define TYPE_TERMINATE 0
#define TYPE_UPDATE    1
#define TYPE_INDEX     2
#define TYPE_CREATE    3

/* These structures are used to buffer control messages
 * that cannot be yet used.
 */

typedef struct snet_ru_entry {
  snet_id_t op_id;
  int node;
  stream_t *stream;
} snet_ru_entry_t;

typedef struct snet_ri_entry {
  snet_id_t op_id;
  int node;
  int index;
} snet_ri_entry_t;


/** @struct imanager_data_t
 *
 *   @brief Input thread's private data
 *
 */

typedef struct {
  stream_t *stream; /**< Output stream. */
  int node;         /**< Node to listen to. */
  int index;        /**< Stream index to use. */
} imanager_data_t;


/* Handle to the master thread.
 *
 * This is needed to make a pthread_join when
 * the system is shut down. Without the join
 * operation MPI_Finalize might be called too early
 * (This is a problem al least with OpenMPI) 
 */
static lpelthread_t *master_thread;

/** <!--********************************************************************-->
 *
 * @fn  static void *IManagerInputThread( void *ptr) 
 *
 *   @brief  Main loop of an input thread
 *
 *           Receives, deserializes and sends records to a stream.
 *
 *   @param ptr  Pointer to input thread's private data.
 *
 *   @return NULL.
 *
 ******************************************************************************/

static void *IManagerInputThread( void *ptr) 
{
  bool terminate = false;

  imanager_data_t *data = (imanager_data_t *)ptr;

  int result;
  int count;
  void *buf;
  int buf_size;

  int position;

  snet_record_t *record;

  MPI_Status status;

  inport_t *iport = InportCreate( data->stream);

  buf_size = ORIGINAL_MAX_MSG_SIZE;

  buf = SNetMemAlloc( sizeof(char) * buf_size);

  while(!terminate) {
    result = MPI_Probe(data->node, data->index, MPI_COMM_WORLD, &status);

    MPI_Get_count(&status, MPI_PACKED, &count);
      
    if(count > buf_size) {
      SNetMemFree(buf);
      buf_size = count;
      buf = SNetMemAlloc( sizeof(char) * buf_size);
    }     
  
    result = MPI_Recv(buf, buf_size, MPI_PACKED, status.MPI_SOURCE, 
		      status.MPI_TAG, MPI_COMM_WORLD, &status);

    position = 0;

    if((record = SNetRecUnpack(MPI_COMM_WORLD, &position, buf, buf_size)) == NULL) {
      SNetUtilDebugFatal("IManager: Could not decode a record!");
    }

    switch( SNetRecGetDescriptor(record)) {
    case REC_data:
    case REC_collect:
    case REC_sort_end:
    case REC_probe: 

      InportWrite( iport, record);

      break;
    case REC_terminate:

      InportWrite( iport, record);

      terminate = true;
      break;
    case REC_sync:
      SNetUtilDebugFatal("IManager: REC_sync should not be passed between nodes!");
    break;
    default:
      SNetUtilDebugFatal("IManager: Decoded unknown record!");
    break;

    }

  }

  InportDestroy( iport);

  SNetMemFree(buf);
  SNetMemFree(data);

  return NULL;
}

/** <!--********************************************************************-->
 *
 * @fn  static void IManagerCreateInput(stream_t *stream, int node, int index)
 *
 *   @brief  Creates a new input thread
 *
 *
 *   @param stream  Stream in which to send records 
 *   @param node    Node where to get the records
 *   @param index   Stream index to use
 *
 ******************************************************************************/

static void IManagerCreateInput(stream_t *stream, int node, int index)
{
  imanager_data_t *data;

  data = SNetMemAlloc(sizeof(imanager_data_t));

  data->stream = stream;
  data->node = node;
  data->index = index;

  //TODO detached
  LpelThreadCreate( IManagerInputThread, (void*)data);
}

/** <!--********************************************************************-->
 *
 * @fn  static void *IManagerCreateNetwork(void *op) 
 *
 *   @brief  Unfold a new part of the network according to the network creation
 *           task described.
 *
 *
 *   @param op  Create network message with information about the network 
 *
 *   @return  NULL
 *
 ******************************************************************************/

static void *IManagerCreateNetwork(void *op) 
{
  stream_t *stream;
  snet_info_t *info;
  snet_startup_fun_t fun;
  snet_msg_create_network_t *msg;

  msg = (snet_msg_create_network_t *)op;

  SNetDistFunID2Fun(&msg->fun_id, &fun);
  
  stream = StreamCreate();

  //SNetTlSetFlag(stream, true);

  info = SNetInfoInit();

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("%lld: Create Network", msg->op_id);
#endif /* DISTRIBUTED_DEBUG */

  SNetInfoSetRoutingContext(info, SNetRoutingContextInit(msg->op_id, false, msg->parent, &msg->fun_id, msg->tag));
  
  stream = fun(stream, info, msg->tag);

  stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), stream);
  
  if(stream != NULL) {
    //stream = SNetTlMarkObsolete(stream);
  }

  SNetInfoDestroy(info);

  SNetMemFree(op);
  
  return NULL;
}

/* Match update message to buffered index message. */
static int IManagerMatchUpdateToIndex(snet_queue_t *queue, snet_id_t op, int node)
{ 
  snet_ri_entry_t *ri;
  snet_queue_iterator_t q_iter;
  int index = INVALID_INDEX;

  q_iter = SNetQueueIteratorBegin(queue);
  
  while((ri = (snet_ri_entry_t *)SNetQueueIteratorPeek(queue, q_iter)) != NULL) {
    
    if((op == ri->op_id) && (node == ri->node)) {
      ri = (snet_ri_entry_t *)SNetQueueIteratorGet(queue, q_iter);
      index = ri->index;
      SNetMemFree(ri);
      break;
    }
    
    q_iter = SNetQueueIteratorNext(queue, q_iter);
  }   

  return index;
}

/* Buffer index message. */
static void IOManagerPutIndexToBuffer(snet_queue_t *queue, snet_id_t op, int node, int index)
{
  snet_ri_entry_t *ri;

  ri = SNetMemAlloc(sizeof(snet_ri_entry_t));
  
  ri->op_id = op;
  ri->node = node;
  ri->index = index;

  SNetQueuePut(queue, ri);
}

/* Match index message to buffered update message. */
static stream_t *IManagerMatchIndexToUpdate(snet_queue_t *queue, snet_id_t op, int node)
{
  
  snet_ru_entry_t *ru;
  snet_queue_iterator_t q_iter;

  stream_t *stream = NULL;

  q_iter = SNetQueueIteratorBegin(queue);
  
  while((ru = (snet_ru_entry_t *)SNetQueueIteratorPeek(queue, q_iter)) != NULL) {
    
    if((op == ru->op_id) && (node == ru->node)) {
      ru = (snet_ru_entry_t *)SNetQueueIteratorGet(queue, q_iter);
      stream = ru->stream;
      SNetMemFree(ru);
      break;
    }
    
    q_iter = SNetQueueIteratorNext(queue, q_iter);
  }   
  
  return stream;
}

/* Buffer update message*/
static void IOManagerPutUpdateToBuffer(snet_queue_t *queue, snet_id_t op, int node, stream_t *stream)
{
  snet_ru_entry_t *ru;

  ru = SNetMemAlloc(sizeof(snet_ru_entry_t));
  
  ru->op_id = op;
  ru->node = node;
  ru->stream = stream;
  
  SNetQueuePut(queue, ru);
}



/** <!--********************************************************************-->
 *
 * @fn  static void *IManagerMasterThread( void *ptr)
 *
 *   @brief  Main loop of the master input thread
 *
 *           Listens and handles control messages.
 *
 *   @param ptr  NULL
 *
 *   @return NULL.
 *
 ******************************************************************************/

static void *IManagerMasterThread( void *ptr) 
{
  bool terminate = false;

  MPI_Status status;
  MPI_Aint true_lb;
  MPI_Aint true_extent;

  MPI_Request requests[NUM_MESSAGE_TYPES];
  char *bufs[NUM_MESSAGE_TYPES];
  int tags[NUM_MESSAGE_TYPES];
  MPI_Datatype types[NUM_MESSAGE_TYPES];

  int index;
  int i;

  int my_rank;

  snet_queue_t *update_queue;
  snet_queue_t *index_queue;

  snet_msg_route_update_t *msg_route_update;
  snet_msg_route_index_t *msg_route_index;
  snet_msg_create_network_t *msg_create_network;

  stream_t *stream = NULL;

#ifdef SOME_NOT_ANY
  int rj, rcount, rlist[NUM_MESSAGE_TYPES]; 
  MPI_Status rstats[NUM_MESSAGE_TYPES];
#else
#endif
  
  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  update_queue = SNetQueueCreate();
  index_queue = SNetQueueCreate();

  /* Init one asynchronous listen call for each type of control message */
  tags[TYPE_TERMINATE] = SNET_msg_terminate;
  tags[TYPE_UPDATE] = SNET_msg_route_update;
  tags[TYPE_INDEX] = SNET_msg_route_index;
  tags[TYPE_CREATE] = SNET_msg_create_network;

  tags[TYPE_TERMINATE] = SNET_msg_terminate;

  for(index = 0; index < NUM_MESSAGE_TYPES; index++) {
    requests[index] = MPI_REQUEST_NULL;

    types[index] = SNetMessageGetMPIType(tags[index]);

    MPI_Type_get_true_extent(types[index], &true_lb, &true_extent);
    
    bufs[index] = SNetMemAlloc(true_extent * 2);
    
    MPI_Irecv(bufs[index], 2, types[index], MPI_ANY_SOURCE, tags[index], MPI_COMM_WORLD, &requests[index]);
  }

  /* Serve messages */

  while(!terminate) {
#ifndef SOME_NOT_ANY
    MPI_Waitany(NUM_MESSAGE_TYPES, requests, &index, &status);
#else 
    MPI_Waitsome(NUM_MESSAGE_TYPES, requests, &rcount, rlist, rstats);
    for( rj=0; rj<rcount; rj++) {
      status = rstats[rj];
      index = rlist[rj];
#endif

    switch(index) {
    case TYPE_TERMINATE: /* Terminate */
      if(status.MPI_SOURCE != my_rank) {
	SNetRoutingNotifyAll();
      }
      
      terminate = true;
      break;
    case TYPE_UPDATE: /* New stream received */

      msg_route_update = (snet_msg_route_update_t *)bufs[index];

      i = IManagerMatchUpdateToIndex(index_queue, 
				     msg_route_update->op_id, 
				     msg_route_update->node);

      if(i != INVALID_INDEX) {
	IManagerCreateInput(msg_route_update->stream, msg_route_update->node, i);

#ifdef DISTRIBUTED_DEBUG
	SNetUtilDebugNotice("Input from %d:%d added",msg_route_update->node , i);
#endif /* DISTRIBUTED_DEBUG */

      } else {
	IOManagerPutUpdateToBuffer(update_queue, 
				   msg_route_update->op_id, 
				   msg_route_update->node, 
				   msg_route_update->stream);
      }

      break;
    case TYPE_INDEX: /* New stream index received */

      msg_route_index = (snet_msg_route_index_t *)bufs[index];

      stream = IManagerMatchIndexToUpdate(update_queue,
					  msg_route_index->op_id, 
					  msg_route_index->node);

      if(stream  != NULL) {
	IManagerCreateInput(stream, msg_route_index->node, msg_route_index->index);

#ifdef DISTRIBUTED_DEBUG
	SNetUtilDebugNotice("Input from %d:%d added", msg_route_index->node, msg_route_index->index);
#endif /* DISTRIBUTED_DEBUG */

      } else {
	IOManagerPutIndexToBuffer(index_queue, 
				  msg_route_index->op_id, 
				  msg_route_index->node, 
				  msg_route_index->index);
      }

      break;
    case TYPE_CREATE: /* Unfold new network */
      msg_create_network = (snet_msg_create_network_t *)bufs[index];

      MPI_Type_get_true_extent(types[index], &true_lb, &true_extent);

      bufs[index] = SNetMemAlloc(true_extent);

      //TODO detached
      LpelThreadCreate( IManagerCreateNetwork, (void*)msg_create_network);
      break;
    default:
      break;
    }
    
    MPI_Irecv(bufs[index], 2, types[index], MPI_ANY_SOURCE, tags[index], MPI_COMM_WORLD, &requests[index]); 
#ifdef SOME_NOT_ANY
    } /* for-loop of Waitsome */
#endif
  } 



  for(index = 0; index < NUM_MESSAGE_TYPES; index++) {
    if(requests[index] != MPI_REQUEST_NULL) {
      MPI_Cancel(&requests[index]);
      MPI_Request_free(&requests[index]);
    }

    SNetMemFree(bufs[index]);
  }

  SNetQueueDestroy(update_queue);
  SNetQueueDestroy(index_queue);

  return NULL;
}

/** <!--********************************************************************-->
 *
 * @fn  void SNetIManagerCreate()
 *
 *   @brief  Create input manager
 *
 *   @note  Call only once!
 *
 *
 ******************************************************************************/

void SNetIManagerCreate()
{
  master_thread = LpelThreadCreate( IManagerMasterThread, NULL);
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetIManagerDestroy()
 *
 *   @brief  Destroys the input manager
 *
 *           The call blocks until the input manager stops.
 *
 *  @note    Call SNetDistributionStop before this!
 *
 *
 ******************************************************************************/

void SNetIManagerDestroy()
{
  LpelThreadJoin( master_thread, NULL);
}

/*@}*/

#endif /* DISTRIBUTED_SNET */
