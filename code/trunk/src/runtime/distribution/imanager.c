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

             
static int compare_fun(void *a, void *b)
{
  return a == b;
}

static int compare_update_to_index(void *a, void *b) 
{
  snet_msg_route_update_t *update = (snet_msg_route_update_t *)a;
  snet_msg_route_index_t * index = (snet_msg_route_index_t *)b;

  return (update->op_id == index->op_id) && (update->node == index->node);

}

static int compare_index_to_update(void *a, void *b) 
{
  snet_msg_route_index_t * index = (snet_msg_route_index_t *)a;
  snet_msg_route_update_t *update = (snet_msg_route_update_t *)b;

  return (index->op_id == update->op_id) && (index->node == update->node);
}

typedef struct {
  snet_queue_t *queue;
  snet_tl_stream_t *stream;
} itable_entry_t;

static itable_entry_t *createEntry(snet_tl_stream_t *stream) 
{
  itable_entry_t *temp = SNetMemAlloc(sizeof(itable_entry_t));

  temp->queue = NULL;
  temp->stream = stream;
  return temp;
}

static void destroyEntry(itable_entry_t *entry) 
{
  if(entry->queue != NULL) { 
    SNetQueueDestroy(entry->queue);
  }

  SNetMemFree(entry);
}

static snet_tl_stream_t *getStream(itable_entry_t *entry)
{
  return entry->stream;
}

static void setStream(itable_entry_t *entry, snet_tl_stream_t *stream)
{
  entry->stream = stream;
}

static void setNextEntry(itable_entry_t *entry, snet_record_t *rec)
{
  if(entry->queue == NULL) {
    entry->queue = SNetQueueCreate(NULL);
  }

  SNetQueuePut(entry->queue, rec);
}

static snet_record_t *getNextEntry(itable_entry_t *entry)
{
  if(entry->queue == NULL) {
    return NULL;
  }

  return SNetQueueGet(entry->queue);
}

static void *IManagerThread( void *ptr) {
  MPI_Status status;
  int my_rank;
  int result;
  snet_record_t *rec;
  snet_record_t *temp_rec;
  snet_tl_stream_t *omanager = (snet_tl_stream_t *)ptr;
  bool terminate = false;
  void *buf;
  int buf_size;

  snet_hashtable_t *routing_table;
  snet_id_t key;

  int count;
  int index;

  snet_fun_id_t fun_id;
  snet_startup_fun_t fun;
  snet_tl_stream_t *stream;

  snet_routing_info_t *info;

  snet_msg_route_update_t *msg_route_update;
  snet_msg_route_index_t *msg_route_index;
  snet_msg_create_network_t *msg_create_network;

  snet_queue_t *update_queue;
  snet_queue_t *index_queue;

  snet_msg_route_update_t *update_temp;
  snet_msg_route_index_t *index_temp;

  itable_entry_t *entry;

  int position;

  MPI_Datatype datatype;
  
  buf_size = ORIGINAL_MAX_MSG_SIZE;

  buf = SNetMemAlloc(sizeof(char) * buf_size);

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  routing_table = SNetHashtableCreate(HASHTABLE_SIZE, compare_fun);

  update_queue = SNetQueueCreate(compare_index_to_update);
  index_queue = SNetQueueCreate(compare_update_to_index);

  while(!terminate) {
    result =  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    switch(status.MPI_TAG) {
    case SNET_msg_record:

      MPI_Get_count(&status, MPI_PACKED, &count);
      
      if(count > buf_size) {
	SNetMemFree(buf);
	buf_size = count;
	buf = SNetMemAlloc(sizeof(char) * buf_size);
      }     
  
      result = MPI_Recv(buf, buf_size, MPI_PACKED, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      position = 0;

      result = MPI_Unpack(buf, buf_size, &position, &index, 1, MPI_INT, MPI_COMM_WORLD);

      if((rec = SNetRecUnpack(MPI_COMM_WORLD, &position, buf, buf_size)) != NULL) {
	
	key = SNET_ID_CREATE(status.MPI_SOURCE, index);

	if((entry = SNetHashtableGet(routing_table, key)) != NULL) {
	  
	  if(getStream(entry) != NULL) {
	    
	    switch( SNetRecGetDescriptor(rec)) {
	    case REC_sync:
	      /* TODO: This is a error! */
	      break;
	    case REC_data:
	    case REC_collect:
	    case REC_sort_begin:	
	    case REC_sort_end:
	    case REC_probe: 
	      SNetTlWrite(getStream(entry), rec);
	      break;
	    case REC_terminate:  
	      SNetTlWrite(getStream(entry), rec);

	      SNetTlMarkObsolete(getStream(entry));

	      entry = SNetHashtableRemove(routing_table, key);

	      destroyEntry(entry);
	      break;
	    case REC_route_update:
	      /* TODO: */
	      break;
	    case REC_route_redirect:
	      /* TODO: */
	      break;     
	    default:
	      /* TODO: This is a error! */
	      printf("IManager: Decoded unknown record!\n");
	      break;
	    }
	  } else { 
	    setNextEntry(entry, rec);
	  }
	} else {
	  entry = createEntry(NULL); 
	  
	  setNextEntry(entry, rec);

	  SNetHashtablePut(routing_table, key, entry);
	}
      } else {
	/* TODO: error - Null record decoded! */
	printf("IManager: Decoded NULL record!\n");
      } 
      
      break; 
      
    case SNET_msg_route_update:
      datatype = SNetMessageGetMPIType(SNET_msg_route_update);

      result = MPI_Recv(buf, 1, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_update = (snet_msg_route_update_t *)buf;

      index_temp =  (snet_msg_route_index_t *)SNetQueueGetMatch(index_queue, msg_route_update);

      if(index_temp != NULL) {
	key = SNET_ID_CREATE(msg_route_update->node, index_temp->index);
	
	entry = createEntry(msg_route_update->stream);

	if(SNetHashtablePut(routing_table, key, entry) != 0) {
	  destroyEntry(entry);

	  entry = SNetHashtableGet(routing_table, key);

	  setStream(entry, msg_route_update->stream);
	  
	  while(entry != NULL && (temp_rec = getNextEntry(entry)) != NULL) {
	    switch(SNetRecGetDescriptor(temp_rec)) {
	    case REC_terminate:
	      SNetTlWrite(msg_route_update->stream, temp_rec);

	      SNetTlMarkObsolete(msg_route_update->stream);

	      entry = SNetHashtableRemove(routing_table, key);

	      destroyEntry(entry);

	      entry = NULL;
	      break;
	    case REC_sync:
	      break;
	    case REC_data:
	    case REC_collect:
	    case REC_sort_begin:	
	    case REC_sort_end:
	    case REC_probe:
 	      SNetTlWrite(msg_route_update->stream, temp_rec);
	    case REC_route_update:
	    case REC_route_redirect:
	    default:
	      break;
	    }
	  }
	}

	SNetMemFree(index_temp);

      } else {
	update_temp = SNetMemAlloc(sizeof(snet_msg_route_update_t));

	update_temp->op_id = msg_route_update->op_id;
	update_temp->node = msg_route_update->node;
	update_temp->stream = msg_route_update->stream;

	SNetQueuePut(update_queue, update_temp);
      }
      break;
    case SNET_msg_route_index:
      datatype = SNetMessageGetMPIType(SNET_msg_route_index);

      result = MPI_Recv(buf, 1, datatype, status.MPI_SOURCE, 
			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_route_index = (snet_msg_route_index_t *)buf;

      update_temp =  (snet_msg_route_update_t *)SNetQueueGetMatch(update_queue, msg_route_index);

      if(update_temp != NULL) {
	key = SNET_ID_CREATE(msg_route_index->node, msg_route_index->index);
	
	entry = createEntry(update_temp->stream);
	
	if(SNetHashtablePut(routing_table, key, entry) != 0) {
	  destroyEntry(entry);

	  entry = SNetHashtableGet(routing_table, key);

	  setStream(entry, update_temp->stream);

	  while(entry != NULL && (temp_rec = getNextEntry(entry)) != NULL) {
	    switch(SNetRecGetDescriptor(temp_rec)) {
	    case REC_terminate:
	      SNetTlWrite(update_temp->stream, temp_rec);

	      SNetTlMarkObsolete(update_temp->stream);

	      entry = SNetHashtableRemove(routing_table, key);

	      destroyEntry(entry);

	      entry = NULL;
	      break;
	    case REC_sync:
	      break;
	    case REC_data:
	    case REC_collect:
	    case REC_sort_begin:	
	    case REC_sort_end:
	    case REC_probe: 
	      SNetTlWrite(update_temp->stream, temp_rec);
	      break;
	    case REC_route_update:
	    case REC_route_redirect:
	    default:
	      break;
	    }
	  }
	}

	SNetMemFree(update_temp);
	
      } else {
	index_temp = SNetMemAlloc(sizeof(snet_msg_route_index_t));

	index_temp->op_id = msg_route_index->op_id;
	index_temp->node = msg_route_index->node;
	index_temp->index = msg_route_index->index;

	SNetQueuePut(index_queue, index_temp);
      }
  
      break;
    case SNET_msg_create_network:
      datatype = SNetMessageGetMPIType(SNET_msg_create_network);

      result = MPI_Recv(buf, 2, datatype, status.MPI_SOURCE, 
      			status.MPI_TAG, MPI_COMM_WORLD, &status);

      msg_create_network = (snet_msg_create_network_t *)buf;

      strcpy(fun_id.lib, msg_create_network->lib);

      fun_id.fun_id = msg_create_network->fun_id;

      SNetDistFunID2Fun(&fun_id, &fun);

      stream = SNetTlCreateUnboundedStream();
      SNetTlSetFlag(stream, true);
     
      /* TODO: Do this in another thread! */

      info = SNetRoutingInfoInit(msg_create_network->op_id, status.MPI_SOURCE, 
				 msg_create_network->parent, &fun_id, 
				 msg_create_network->tag);

      stream = SNetRoutingInfoFinalize(info, fun(stream, info, msg_create_network->tag));

      SNetRoutingInfoDestroy(info);

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
