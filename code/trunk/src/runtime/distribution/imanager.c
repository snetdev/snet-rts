
#ifdef DISTRIBUTED_SNET
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
#include "stream_layer.h"
#include "fun.h"
#include "id.h"
#include "queue.h"
#include "debug.h"

/* TODO: 
 *  - Fetch of network description?
 */

#define ORIGINAL_MAX_MSG_SIZE 256  
#define INVALID_INDEX -1

typedef struct snet_ru_entry {
  int op_id;
  int node;
  snet_tl_stream_t *stream;
} snet_ru_entry_t;

typedef struct snet_ri_entry {
  int op_id;
  int node;
  int index;
} snet_ri_entry_t;

typedef struct {
  snet_tl_stream_t *stream;
  int node;
  int index;
} imanager_data_t;


/* Handle to the master thread.
 *
 * This is needed to make a pthread_join when
 * the system is shut down. Without the join
 * operation MPI_Finalize might be called too early
 * (This is a problem al least with OpenMPI) 
 */

static pthread_t master_thread;

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


  buf_size = ORIGINAL_MAX_MSG_SIZE;

  buf = SNetMemAlloc(sizeof(char) * buf_size);

  while(!terminate) {
    result = MPI_Probe(data->node, data->index, MPI_COMM_WORLD, &status);

    MPI_Get_count(&status, MPI_PACKED, &count);
      
    if(count > buf_size) {
      SNetMemFree(buf);
      buf_size = count;
      buf = SNetMemAlloc(sizeof(char) * buf_size);
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
    case REC_sort_begin:	
    case REC_sort_end:
    case REC_probe: 

      SNetTlWrite(data->stream, record);

      break;
    case REC_terminate:

      SNetTlWrite(data->stream, record);
      SNetTlMarkObsolete(data->stream);

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

  SNetMemFree(buf);
  SNetMemFree(data);

  return NULL;
}

static void IManagerCreateInput(snet_tl_stream_t *stream, int node, int index)
{
  pthread_t thread;
  imanager_data_t *data;

  data = SNetMemAlloc(sizeof(imanager_data_t));

  data->stream = stream;
  data->node = node;
  data->index = index;

  pthread_create(&thread, NULL, IManagerInputThread, (void *)data);
  pthread_detach(thread);

  return;
}

static void *IManagerCreateNetwork(void *op) 
{
  snet_tl_stream_t *stream;
  snet_info_t *info;
  snet_startup_fun_t fun;
  snet_msg_create_network_t *msg;

  msg = (snet_msg_create_network_t *)op;

  SNetDistFunID2Fun(&msg->fun_id, &fun);
  
  stream = SNetTlCreateStream(BUFFER_SIZE);

  SNetTlSetFlag(stream, true);

  info = SNetInfoInit();

  SNetInfoSetRoutingContext(info, SNetRoutingContextInit(msg->op_id, false, msg->parent, &msg->fun_id, msg->tag));
  
  stream = fun(stream, info, msg->tag);

  stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), stream);
  
  stream = SNetTlMarkObsolete(stream);

  SNetInfoDestroy(info);

  SNetMemFree(op);
  
  return NULL;
}
static int IManagerMatchUpdateToIndex(snet_queue_t *queue, int op, int node)
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

static void IOManagerPutIndexToBuffer(snet_queue_t *queue, int op, int node, int index)
{
  snet_ri_entry_t *ri;

  ri = SNetMemAlloc(sizeof(snet_ri_entry_t));
  
  ri->op_id = op;
  ri->node = node;
  ri->index = index;

  SNetQueuePut(queue, ri);
}

static snet_tl_stream_t *IManagerMatchIndexToUpdate(snet_queue_t *queue, int op, int node)
{
  
  snet_ru_entry_t *ru;
  snet_queue_iterator_t q_iter;

  snet_tl_stream_t *stream = NULL;

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

static void IOManagerPutUpdateToBuffer(snet_queue_t *queue, int op, int node, snet_tl_stream_t *stream)
{
  snet_ru_entry_t *ru;

  ru = SNetMemAlloc(sizeof(snet_ru_entry_t));
  
  ru->op_id = op;
  ru->node = node;
  ru->stream = stream;
  
  SNetQueuePut(queue, ru);
}

#define NUM_MESSAGE_TYPES 4

#define TYPE_TERMINATE 0
#define TYPE_UPDATE 1
#define TYPE_INDEX 2
#define TYPE_CREATE 3

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

  pthread_t thread;

  int my_rank;

  snet_queue_t *update_queue;
  snet_queue_t *index_queue;

  snet_msg_route_update_t *msg_route_update;
  snet_msg_route_index_t *msg_route_index;
  snet_msg_create_network_t *msg_create_network;

  snet_tl_stream_t * stream = NULL;

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  update_queue = SNetQueueCreate();
  index_queue = SNetQueueCreate();

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

  while(!terminate) {
    MPI_Waitany(NUM_MESSAGE_TYPES, requests, &index, &status);

    switch(index) {
    case TYPE_TERMINATE:
      if(status.MPI_SOURCE != my_rank) {
	SNetRoutingNotifyAll();
      }
      
      terminate = true;
      break;
    case TYPE_UPDATE:

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
    case TYPE_INDEX:

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
    case TYPE_CREATE:
      msg_create_network = (snet_msg_create_network_t *)bufs[index];

      MPI_Type_get_true_extent(types[index], &true_lb, &true_extent);

      bufs[index] = SNetMemAlloc(true_extent);

      pthread_create(&thread, NULL, IManagerCreateNetwork, (void *)msg_create_network); 
      pthread_detach(thread);
      break;
    default:
      break;
    }
    
    MPI_Irecv(bufs[index], 2, types[index], MPI_ANY_SOURCE, tags[index], MPI_COMM_WORLD, &requests[index]); 
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

void SNetIManagerCreate()
{
  pthread_create(&master_thread, NULL, IManagerMasterThread, NULL);

  return;
}

void SNetIManagerDestroy()
{
  pthread_join(master_thread, NULL);

  return;
}



#ifdef DUMMY
/* NOTICE:
 *
 * This is the old code for input manager, saved only as re-routing capacility is not yet added to
 * the current version. Otherwise obsolete.
 *
 */


//#define IMANAGER_DEBUG
#define DIST_IO_OPTIMIZATIONS

#define HASHTABLE_SIZE 32
#define ORIGINAL_MAX_MSG_SIZE 256  
#define INVALID_INDEX -1

#ifdef IMANAGER_DEBUG
static int debug_record_count = 0;
static int debug_redirection_count = 0;
static int debug_concatenation_count = 0;
#endif /* IMANAGER_DEBUG */

typedef struct snet_ru_entry {
  int op_id;
  int node;
  snet_tl_stream_t *stream;
} snet_ru_entry_t;

typedef struct snet_ri_entry {
  int op_id;
  int node;
  int index;
} snet_ri_entry_t;

typedef struct snet_rec_entry {
  int node;
  int index;
  snet_queue_t *recs;
} snet_rec_entry_t; 

static int compare_fun(void *a, void *b)
{
  return a == b;
}


static void *IManagerCreateNetwork(void *op) 
{
  snet_tl_stream_t *stream;
  snet_routing_info_t *info;
  snet_startup_fun_t fun;
  snet_msg_create_network_t *msg;

  msg = (snet_msg_create_network_t *)op;
  
  SNetDistFunID2Fun(&msg->fun_id, &fun);
  
  stream = SNetTlCreateUnboundedStream();

  SNetTlSetFlag(stream, true);
    
  info = SNetRoutingInfoInit(msg->op_id, msg->starter, msg->parent, &msg->fun_id, msg->tag);
  
  stream = SNetRoutingInfoFinalize(info, fun(stream, info, msg->tag));
  
  SNetRoutingInfoDestroy(info);

  SNetMemFree(op);
  
  return NULL;
}

static void IManagerSendBufferedRecords(snet_queue_t *record_queue, snet_tl_stream_t *stream, int node, int index, snet_hashtable_t *routing_table, snet_id_t key, int my_rank, snet_tl_stream_t *omanager);

/* return true if the routing table entry can be removed: */
static void IManagerPutRecordToStream(snet_record_t *rec, snet_tl_stream_t *stream, snet_hashtable_t *routing_table, snet_id_t key, snet_queue_t *record_queue, int my_rank, snet_tl_stream_t *omanager) 
{
#ifdef DIST_IO_OPTIMIZATIONS
  int node;
  int index;
#endif /* DIST_IO_OPTIMIZATIONS */

  switch( SNetRecGetDescriptor(rec)) {
  case REC_data:
  case REC_collect:
  case REC_sort_begin:	
  case REC_sort_end:
  case REC_probe: 
#ifdef IMANAGER_DEBUG
    debug_record_count++;
#endif /* IMANAGER_DEBUG */
    SNetTlWrite(stream, rec);
    break;
  case REC_terminate:
#ifdef IMANAGER_DEBUG
    debug_record_count++;
#endif /* IMANAGER_DEBUG */
    SNetTlWrite(stream, rec);
    SNetTlMarkObsolete(stream);

    stream = SNetHashtableRemove(routing_table, key);

    break;
#ifdef DIST_IO_OPTIMIZATIONS
  case REC_route_redirect:
#ifdef IMANAGER_DEBUG
    debug_record_count++;
#endif /* IMANAGER_DEBUG */

    node = SNetRecGetNode(rec);
    index = SNetRecGetIndex(rec);

    if(node == -1 && index == -1) {
      SNetTlWrite(omanager, rec);

    } else if(node == SNET_ID_GET_NODE(key) && index == SNET_ID_GET_ID(key)) {
      SNetTlWrite(stream, rec);
      
      SNetTlMarkObsolete(stream); 
      
      stream = SNetHashtableRemove(routing_table, key);
    } else {
#ifdef IMANAGER_DEBUG
      debug_redirection_count++;
#endif /* IMANAGER_DEBUG */
      stream = (snet_tl_stream_t *)SNetHashtableRemove(routing_table, key);
  
      key = SNET_ID_CREATE(node, index);

      SNetHashtablePut(routing_table, key, (void *)stream);

      SNetRecDestroy(rec);

      IManagerSendBufferedRecords(record_queue, stream, node, index, routing_table, key, my_rank, omanager);

    }
    break; 
#endif /* DIST_IO_OPTIMIZATIONS */
  case REC_sync:
  case REC_route_update:
  default:
    SNetUtilDebugFatal("IManager: Decoded unsupported record!\n");
    break;
  }
}

/* return true if the routing table entry can be removed: */
static void IManagerSendBufferedRecords(snet_queue_t *record_queue, snet_tl_stream_t *stream, int node, int index, snet_hashtable_t *routing_table, snet_id_t key, int my_rank, snet_tl_stream_t *omanager)
{
  snet_queue_iterator_t q_iter;
  snet_rec_entry_t *rec_entry;
  snet_record_t *rec;
  
  q_iter = SNetQueueIteratorBegin(record_queue);

  while((rec_entry = (snet_rec_entry_t *)SNetQueueIteratorPeek(record_queue, q_iter)) != NULL) {
    
    if((rec_entry->node == node) && (rec_entry->index == index)) {
      break;
    }
    
    q_iter = SNetQueueIteratorNext(record_queue, q_iter);
  }   
  
  if(rec_entry != NULL) {
    rec_entry = (snet_rec_entry_t *)SNetQueueIteratorGet(record_queue, q_iter);

    while((rec = (snet_record_t *)SNetQueueGet(rec_entry->recs)) != NULL) {
      IManagerPutRecordToStream(rec, stream, routing_table, key, record_queue, my_rank, omanager);
    }
    
    SNetQueueDestroy(rec_entry->recs);
    SNetMemFree(rec_entry);
  }
}

static void IManagerPutRecordToBuffer(snet_queue_t *record_queue, snet_record_t *rec, int node, int index) 
{
  snet_queue_iterator_t q_iter;
  snet_rec_entry_t *rec_entry;

  q_iter = SNetQueueIteratorBegin(record_queue);
  
  while((rec_entry = (snet_rec_entry_t *)SNetQueueIteratorPeek(record_queue, q_iter)) != NULL) {

    if((rec_entry->node == node) && (rec_entry->index == index)) {
      break;
    }
    
    q_iter = SNetQueueIteratorNext(record_queue, q_iter);
  }   

  
  if(rec_entry != NULL) {
    SNetQueuePut(rec_entry->recs, rec);
  } else {
    rec_entry = SNetMemAlloc(sizeof(snet_rec_entry_t));
    
    rec_entry->node = node;
    rec_entry->index = index;
    rec_entry->recs = SNetQueueCreate();
    SNetQueuePut(rec_entry->recs, rec);
    SNetQueuePut(record_queue, rec_entry);
  }

  return;
}

static int IManagerMatchUpdateToIndex(snet_queue_t *queue, int op, int node)
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

static void IOManagerPutIndexToBuffer(snet_queue_t *queue, int op, int node, int index)
{
  snet_ri_entry_t *ri;

  ri = SNetMemAlloc(sizeof(snet_ri_entry_t));
  
  ri->op_id = op;
  ri->node = node;
  ri->index = index;

  SNetQueuePut(queue, ri);
}

static snet_tl_stream_t *IManagerMatchIndexToUpdate(snet_queue_t *queue, int op, int node)
{
  
  snet_ru_entry_t *ru;
  snet_queue_iterator_t q_iter;

  snet_tl_stream_t *stream = NULL;

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

static void IOManagerPutUpdateToBuffer(snet_queue_t *queue, int op, int node, snet_tl_stream_t *stream)
{
  snet_ru_entry_t *ru;

  ru = SNetMemAlloc(sizeof(snet_ru_entry_t));
  
  ru->op_id = op;
  ru->node = node;
  ru->stream = stream;
  
  SNetQueuePut(queue, ru);
}

static void *IManagerThread( void *ptr) {
  MPI_Status status;
  MPI_Aint true_lb;
  MPI_Aint true_extent;
  MPI_Datatype datatype;

  int my_rank;
  int result;
  snet_record_t *rec;

  snet_tl_stream_t *omanager = (snet_tl_stream_t *)ptr;
  bool terminate = false;
  void *buf;
  int buf_size;

  snet_hashtable_t *routing_table;
  snet_id_t key;

  int count;
  int index;

  int position;

  pthread_t thread;

  snet_tl_stream_t *stream;

  snet_queue_t *update_queue;
  snet_queue_t *index_queue;
  snet_queue_t *record_queue;

  /* These are used with 'buf' as templetes to different messages. */
  snet_msg_route_update_t *msg_route_update;
  snet_msg_route_index_t *msg_route_index;
  snet_msg_create_network_t *msg_create_network;

#ifdef DIST_IO_OPTIMIZATIONS
  snet_msg_route_concatenate_t *msg_route_concatenate;
  snet_msg_route_redirect_t *msg_route_redirect;
  snet_msg_route_redirect_internal_t *msg_route_redirect_internal;
#endif /* DIST_IO_OPTIMIZATIONS */


  buf_size = ORIGINAL_MAX_MSG_SIZE;

  buf = SNetMemAlloc(sizeof(char) * buf_size);

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  routing_table = SNetHashtableCreate(HASHTABLE_SIZE, compare_fun);

  update_queue = SNetQueueCreate();
  index_queue = SNetQueueCreate();
  record_queue = SNetQueueCreate();


  while(!terminate) {

    result =  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    switch(status.MPI_TAG) {
    case SNET_msg_record:

      /* Records have varying size, increase buffer size 
       * if the record doesn't fit in it: 
       */

      MPI_Get_count(&status, MPI_PACKED, &count);
      
      if(count > buf_size) {
	SNetMemFree(buf);
	buf_size = count;
	buf = SNetMemAlloc(sizeof(char) * buf_size);
      }     
  
      result = MPI_Recv(buf, buf_size, MPI_PACKED, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      position = 0;

      /* Stream index is stored in front of the record: */
      result = MPI_Unpack(buf, buf_size, &position, &index, 1, MPI_INT, MPI_COMM_WORLD);


      if((rec = SNetRecUnpack(MPI_COMM_WORLD, &position, buf, buf_size)) != NULL) {

	key = SNET_ID_CREATE(status.MPI_SOURCE, index);

	if((stream = SNetHashtableGet(routing_table, key)) != NULL) { 

	  IManagerPutRecordToStream(rec, stream, routing_table, key, record_queue, my_rank, omanager); 
	} else {
	  /* No stream yet, let's buffer this record until there is one. */
	  IManagerPutRecordToBuffer(record_queue, rec, status.MPI_SOURCE, index); 
	}
      } else {
	/* TODO: error - Null record decoded! */
      } 
      
      break; 
      
    case SNET_msg_route_update:

      datatype = SNetMessageGetMPIType(SNET_msg_route_update);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_update = (snet_msg_route_update_t *)buf;

      index = IManagerMatchUpdateToIndex(index_queue, 
					 msg_route_update->op_id, 
					 msg_route_update->node);

      if(index != INVALID_INDEX) {

	key = SNET_ID_CREATE(msg_route_update->node, index);
	
	if(SNetHashtablePut(routing_table, key, msg_route_update->stream) == 0) {

	  IManagerSendBufferedRecords(record_queue, 
				      msg_route_update->stream, 
				      msg_route_update->node, 
				      index,
				      routing_table,
				      key,
				      my_rank,
				      omanager);

	} 
      } else {
	IOManagerPutUpdateToBuffer(update_queue, 
				   msg_route_update->op_id, 
				   msg_route_update->node, 
				   msg_route_update->stream);
      }

      break;
    case SNET_msg_route_index:
      datatype = SNetMessageGetMPIType(SNET_msg_route_index);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_index = (snet_msg_route_index_t *)buf;

      stream = IManagerMatchIndexToUpdate(update_queue,
					  msg_route_index->op_id, 
					  msg_route_index->node);

      if(stream  != NULL) {
	key = SNET_ID_CREATE(msg_route_index->node, msg_route_index->index);
	
	if(SNetHashtablePut(routing_table, key, stream) == 0) {

	  IManagerSendBufferedRecords(record_queue, 
				      stream, 
				      msg_route_index->node, 
				      msg_route_index->index,
				      routing_table,
				      key,
				      my_rank,
				      omanager);	  
	}
      } else {
	IOManagerPutIndexToBuffer(index_queue, 
				  msg_route_index->op_id, 
				  msg_route_index->node, 
				  msg_route_index->index);
      }

      break;
    case SNET_msg_create_network:
      datatype = SNetMessageGetMPIType(SNET_msg_create_network);

      result = MPI_Type_get_true_extent(datatype, &true_lb, &true_extent);

      msg_create_network = SNetMemAlloc(true_extent);

      result = MPI_Recv(msg_create_network, 2, datatype, status.MPI_SOURCE, 
      			status.MPI_TAG, MPI_COMM_WORLD, &status);

      pthread_create(&thread, NULL, IManagerCreateNetwork, (void *)msg_create_network); 
      pthread_detach(thread);

      break;
    case SNET_msg_terminate:
      result = MPI_Recv(buf, 0, MPI_INT, status.MPI_SOURCE, 
			SNET_msg_terminate, MPI_COMM_WORLD, &status);

      SNetTlWrite(omanager, SNetRecCreate(REC_terminate));
      SNetTlMarkObsolete(omanager);
      if(status.MPI_SOURCE != my_rank) {
	SNetRoutingNotifyAll();
      }
      
      terminate = true;
      break;
#ifdef DIST_IO_OPTIMIZATIONS
    case SNET_msg_route_concatenate:

      datatype = SNetMessageGetMPIType(SNET_msg_route_concatenate);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_concatenate = (snet_msg_route_concatenate_t *)buf;
      
      key = SNET_ID_CREATE(my_rank, msg_route_concatenate->index);

      if((stream = SNetHashtableRemove(routing_table, key)) != NULL) { 

      	SNetTlWrite(stream, SNetRecCreate(REC_sync, msg_route_concatenate->stream));
      	SNetTlMarkObsolete(stream);
#ifdef IMANAGER_DEBUG
	debug_concatenation_count++;
#endif /* IMANAGER_DEBUG */
      } else {
	
	SNetTlWrite(omanager, SNetRecCreate(REC_route_concatenate,
					    msg_route_concatenate->index,
					    msg_route_concatenate->stream));	
      }

      break;
    case SNET_msg_route_redirect_internal:
      datatype = SNetMessageGetMPIType(SNET_msg_route_redirect_internal);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_redirect_internal = (snet_msg_route_redirect_internal_t *)buf;


      key = SNetHashtableGetKey(routing_table, msg_route_redirect_internal->stream);

      if(key != UINT64_MAX) {
	msg_route_redirect = (snet_msg_route_redirect_t *)buf;

	msg_route_redirect->node = msg_route_redirect_internal->node;
	msg_route_redirect->index = SNET_ID_GET_ID(key);

	datatype = SNetMessageGetMPIType(SNET_msg_route_redirect);
	
	MPI_Send(msg_route_redirect, 1, datatype, SNET_ID_GET_NODE(key), 
		 SNET_msg_route_redirect, MPI_COMM_WORLD);
      } else {
	SNetTlWrite(omanager, SNetRecCreate(REC_route_redirect, -1, -1));
      }
      break;
    case SNET_msg_route_redirect:

      datatype = SNetMessageGetMPIType(SNET_msg_route_redirect);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_redirect = (snet_msg_route_redirect_t *)buf;

      SNetTlWrite(omanager, SNetRecCreate(REC_route_redirect, msg_route_redirect->node,
					  msg_route_redirect->index));
   
      break;
#endif /* DIST_IO_OPTIMIZATIONS */
    default:
      // ERROR: This should never happen!
      break;
    }
  }

  SNetMemFree(buf); 
  SNetQueueDestroy(update_queue);
  SNetQueueDestroy(index_queue);
  SNetQueueDestroy(record_queue);
  SNetHashtableDestroy(routing_table);

#ifdef IMANAGER_DEBUG
  SNetUtilDebugNotice("Node %d: imanager received %d records", my_rank, debug_record_count);
  SNetUtilDebugNotice("Node %d: %d redirections", my_rank, debug_redirection_count);
  SNetUtilDebugNotice("Node %d: %d concatenations", my_rank, debug_concatenation_count);
#endif /* IMANAGER_DEBUG */

  return NULL;
}


void IManagerCreate(snet_tl_stream_t *omngr)
{
  pthread_t thread;

  pthread_create(&thread, NULL, IManagerThread, (void *)omngr);
  pthread_detach(thread);
}

#endif /* DUMMY */
#endif /* DISTRIBUTED_SNET */
