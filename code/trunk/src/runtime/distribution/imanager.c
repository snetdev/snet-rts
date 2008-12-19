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
#include "hashtable.h"
#include "id.h"
#include "queue.h"

/* TODO: 
 *  - Route redirection
 *  - Route concatenation
 *  - Fetch of network description
 */

#define HASHTABLE_SIZE 32
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

/* return true if the routing table entry can be removed: */
static bool IManagerPutRecordToStream(snet_record_t *rec, snet_tl_stream_t *stream) 
{
  bool ret = false;

  switch( SNetRecGetDescriptor(rec)) {
  case REC_sync:
    /* TODO: This is an error! */
    break;
  case REC_data:
  case REC_collect:
  case REC_sort_begin:	
  case REC_sort_end:
  case REC_probe: 
    SNetTlWrite(stream, rec);
    break;
  case REC_terminate:  
    SNetTlWrite(stream, rec);
    
    SNetTlMarkObsolete(stream);
    
    ret = true;
    break;
  case REC_route_update:
    /* TODO: This is an error! */
    break;
  case REC_route_redirect:
    /* TODO: */
    break;     
  default:
    /* TODO: This is a error! */
    printf("IManager: Decoded unknown record!\n");
    break;
  }

  return ret;
}

/* return true if the routing table entry can be removed: */
static bool IManagerSendBufferedRecords(snet_queue_t *record_queue, snet_tl_stream_t *stream, int node, int index) 
{
  snet_queue_iterator_t q_iter;
  snet_rec_entry_t *rec_entry;
  snet_record_t *rec;
  bool ret = false;
  
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
      
      if(IManagerPutRecordToStream(rec, stream)) {
	ret = true;
      } 
    }
    
    SNetQueueDestroy(rec_entry->recs);
    SNetMemFree(rec_entry);
  }

  return ret;
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

  MPI_Datatype datatype;
  
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
	  if(IManagerPutRecordToStream(rec, stream)) {
	    SNetHashtableRemove(routing_table, key);
	  } 
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

      result = MPI_Recv(buf, 1, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_update = (snet_msg_route_update_t *)buf;

      index = IManagerMatchUpdateToIndex(index_queue, 
					 msg_route_update->op_id, 
					 msg_route_update->node);

      if(index != INVALID_INDEX) {

	key = SNET_ID_CREATE(msg_route_update->node, index);
	
	if(SNetHashtablePut(routing_table, key, msg_route_update->stream) == 0) {

	  if(IManagerSendBufferedRecords(record_queue, 
					 msg_route_update->stream, 
					 msg_route_update->node, 
					 index)) {

	    SNetHashtableRemove(routing_table, key);
	  } 

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

      result = MPI_Recv(buf, 1, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_index = (snet_msg_route_index_t *)buf;

      stream = IManagerMatchIndexToUpdate(update_queue,
					  msg_route_index->op_id, 
					  msg_route_index->node);

      if(stream  != NULL) {
	key = SNET_ID_CREATE(msg_route_index->node, msg_route_index->index);
	
	if(SNetHashtablePut(routing_table, key, stream) == 0) {	
  
	  if(IManagerSendBufferedRecords(record_queue, 
					 stream, 
					 msg_route_index->node, 
					 msg_route_index->index)) {

	    SNetHashtableRemove(routing_table, key);
	  } 
	  
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

      result = MPI_Recv(msg_create_network, 1, datatype, status.MPI_SOURCE, 
      			status.MPI_TAG, MPI_COMM_WORLD, &status);

      pthread_create(&thread, NULL, IManagerCreateNetwork, (void *)msg_create_network); 
      pthread_detach(thread);

      break;
    case SNET_msg_terminate:
      SNetTlWrite(omanager, SNetRecCreate(REC_terminate));
      SNetTlMarkObsolete(omanager);
      if(status.MPI_SOURCE != my_rank) {
	SNetRoutingNotifyAll();
      }
      
      terminate = true;
      break;
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

  return NULL;
}


void IManagerCreate(snet_tl_stream_t *omngr)
{
  pthread_t thread;

  pthread_create(&thread, NULL, IManagerThread, (void *)omngr);
  pthread_detach(thread);
}

#endif /* DISTRIBUTED_SNET */
