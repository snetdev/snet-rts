#ifdef DISTRIBUTED_SNET
#include <pthread.h>
#include <mpi.h>
#include <stdio.h>
#include "unistd.h"

#include "omanager.h"
#include "record.h"
#include "stream_layer.h"
#include "memfun.h"
#include "message.h"
#include "debug.h"

#define ORIGINAL_MAX_MSG_SIZE 256

typedef struct {
  snet_tl_stream_t *stream;
  int node;
  int index;
} omanager_data_t;

static void *OManagerThread(void *ptr)
{
  bool terminate = false;

  omanager_data_t *data = (omanager_data_t *)ptr;

  snet_record_t *record;

  int position;
  int *buf;
  int buf_size;

  buf_size = ORIGINAL_MAX_MSG_SIZE;
  buf = SNetMemAlloc(sizeof(char) * buf_size);

  while(!terminate) {

    record = SNetTlRead(data->stream);

    switch(SNetRecGetDescriptor(record)) {
    case REC_sync:

      data->stream = SNetRecGetStream(record);

      SNetRecDestroy(record);
      break;
    case REC_data:
    case REC_collect:
    case REC_sort_begin:	
    case REC_sort_end:
    case REC_probe:
      position = 0;
     
      while(SNetRecPack(record, MPI_COMM_WORLD, &position, buf, buf_size) != MPI_SUCCESS) {
	
	SNetMemFree(buf);
	
	buf_size += ORIGINAL_MAX_MSG_SIZE;
	
	buf = SNetMemAlloc(sizeof(char) * buf_size);
	
	position = 0;

      }
      
      MPI_Send(buf, position, MPI_PACKED, data->node, data->index, MPI_COMM_WORLD);
     
      SNetRecDestroy(record);
      break;
    case REC_terminate:
      terminate = true;

      /* No need to check for message size, as buf_size will always be greater! */

      position = 0;

      SNetRecPack(record, MPI_COMM_WORLD, &position, buf, buf_size);

      MPI_Send(buf, position, MPI_PACKED, data->node, data->index, MPI_COMM_WORLD);

      SNetRecDestroy(record);
      break;
    default:
      SNetUtilDebugFatal("OManager: Unknown record received!");
      break;
    }


  }

  SNetMemFree(buf);
  SNetMemFree(data);

  return NULL;
}

void SNetOManagerUpdateRoutingTable(snet_tl_stream_t *stream, int node, int index)
{
  pthread_t thread;
  omanager_data_t *data;

#ifdef DISTRIBUTED_DEBUG
  int my_rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  SNetUtilDebugNotice("Output to %d:%d added", node , index);
#endif /* DISTRIBUTED_DEBUG */

  data = SNetMemAlloc(sizeof(omanager_data_t));

  data->stream = stream;
  data->node = node;
  data->index = index;
  
  pthread_create(&thread, NULL, OManagerThread, (void *)data);
  pthread_detach(thread);
  
  return;
}

#ifdef DUMMY
/* NOTICE:
 *
 * This is the old code for output manager, saved only as re-routing capacility is not yet added to
 * the current version. Otherwise obsolete.
 *
 */

//#define OMANAGER_DEBUG
#define DIST_IO_OPTIMIZATIONS

#define HASHTABLE_SIZE 32
#define ORIGINAL_MAX_MSG_SIZE 256

typedef struct {
  int node;
  int id;
} pair_t;
     
static int compare_fun(void *b, void *a) {
  pair_t *a2 = (pair_t *)a;

  return  (a2->id == *(int *)b);
}

static pair_t *CreatePair(int node, int id) 
{
  pair_t *pair;

  pair = SNetMemAlloc(sizeof(pair_t));

  pair->node = node;
  pair->id = id;

  return pair;
}

static void DestroyPair(pair_t *pair) {
  SNetMemFree(pair);
}

static void *OManagerThread( void *ptr) 
{
  snet_tl_stream_t *imanager = (snet_tl_stream_t *)ptr;
  snet_tl_streamset_t *set;
  bool terminate = false;

  snet_hashtable_t * routing_table;
  pair_t *value_pair;
  uint64_t key;

  snet_tl_stream_record_pair_t pair;
  snet_tl_stream_t *stream;

  int my_rank;
  int *buf;
  int buf_size;

  int position;
  int result;

#ifdef DIST_IO_OPTIMIZATIONS 
  MPI_Datatype type;

  bool undo = false;
  int index;
  int node;

  snet_msg_route_concatenate_t msg_concat;
  snet_msg_route_redirect_internal_t msg_redir;

  snet_tl_stream_t *redirected = NULL;
#endif /* DIST_IO_OPTIMIZATIONS */

#ifdef OMANAGER_DEBUG
  int debug_record_count = 0;
#endif /* OMANAGER_DEBUG */

  routing_table = SNetHashtableCreate(HASHTABLE_SIZE, compare_fun);

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  buf_size = ORIGINAL_MAX_MSG_SIZE;
  buf = SNetMemAlloc(sizeof(char) * buf_size);

  set = SNetTlCreateStreamset();

  set = SNetTlAddToStreamset(set, imanager);

  while(!terminate) {

#ifndef DIST_IO_OPTIMIZATIONS 
    pair = SNetTlReadMany(set);

#else /* DIST_IO_OPTIMIZATIONS */
    pair = SNetTlPeekMany(set);
#endif /* DIST_IO_OPTIMIZATIONS */

    switch(SNetRecGetDescriptor(pair.record)) {
    case REC_sync:

#ifdef DIST_IO_OPTIMIZATIONS 
      pair.record = SNetTlRead(pair.stream);
#endif /* DIST_IO_OPTIMIZATIONS */

      stream = SNetRecGetStream(pair.record);

      key = HASHTABLE_PTR_TO_KEY(pair.stream);

      value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);

      key = HASHTABLE_PTR_TO_KEY(stream);

      SNetHashtablePut(routing_table, key, value_pair);    

      set = SNetTlReplaceInStreamset(set, pair.stream, stream);

      SNetRecDestroy(pair.record);

#ifdef DIST_IO_OPTIMIZATIONS
      if(SNetTlGetFlag(stream) && redirected == NULL && stream != imanager) {

	/* Empty stream detected! */

	msg_redir.node = value_pair->node;
	msg_redir.stream = stream;
	
	type = SNetMessageGetMPIType(SNET_msg_route_redirect_internal);
	
	MPI_Send(&msg_redir, 1, type, my_rank, SNET_msg_route_redirect_internal, MPI_COMM_WORLD);
	
	redirected = stream;
      }
#endif /* DIST_IO_OPTIMIZATIONS */
      break;
    case REC_data:
    case REC_collect:
    case REC_sort_begin:	
    case REC_sort_end:
    case REC_probe:
#ifdef OMANAGER_DEBUG
      debug_record_count++;
#endif /* OMANAGER_DEBUG */

      key = HASHTABLE_PTR_TO_KEY(pair.stream);

      value_pair = (pair_t *)SNetHashtableGet(routing_table, key); 

#ifdef DIST_IO_OPTIMIZATIONS
      if((undo == false) && (value_pair->node == my_rank) && (redirected == NULL) && (pair.stream != imanager)) {
	/* Fragmentation detected: concatenate streams! */

	msg_concat.index = value_pair->id;
	msg_concat.stream = pair.stream;

	type = SNetMessageGetMPIType(SNET_msg_route_concatenate);
	
	value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);

	set = SNetTlDeleteFromStreamset(set, pair.stream);

	MPI_Send(&msg_concat, 1, type, my_rank, SNET_msg_route_concatenate, MPI_COMM_WORLD);

	DestroyPair(value_pair);

#ifdef OMANAGER_DEBUG
	debug_record_count--;
#endif /* OMANAGER_DEBUG */
      } else {
        if(undo == true) {
	  undo = false;
	}

	pair.record = SNetTlRead(pair.stream);

#endif /* DIST_IO_OPTIMIZATIONS */

	position = 0;
	
	result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);
	
	while(SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size) != MPI_SUCCESS) {

	  SNetMemFree(buf);
	  
	  buf_size += ORIGINAL_MAX_MSG_SIZE;
	  
	  buf = SNetMemAlloc(sizeof(char) * buf_size);
	  
	  position = 0;

	  result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);
	}

	MPI_Send(buf, position, MPI_PACKED, value_pair->node, SNET_msg_record, MPI_COMM_WORLD);

	SNetRecDestroy(pair.record);


#ifdef DIST_IO_OPTIMIZATIONS
      }
#endif /* DIST_IO_OPTIMIZATIONS */

      break;
    case REC_terminate:
#ifdef DIST_IO_OPTIMIZATIONS 
	pair.record = SNetTlRead(pair.stream);
#endif /* DIST_IO_OPTIMIZATIONS */

      if(pair.stream == imanager) {
	
	terminate = true;

	set = SNetTlDeleteFromStreamset(set, pair.stream);

      } else {
	key = HASHTABLE_PTR_TO_KEY(pair.stream);

	value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);


	set = SNetTlDeleteFromStreamset(set, pair.stream);

	/* No need to check for message size, as buf_size will always be greater! */

	position = 0;

	result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);


	SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size);

	MPI_Send(buf, position, MPI_PACKED, value_pair->node, SNET_msg_record, MPI_COMM_WORLD);

	DestroyPair(value_pair);

#ifdef OMANAGER_DEBUG
	debug_record_count++;
#endif /* OMANAGER_DEBUG */
      }
      SNetRecDestroy(pair.record);

      break;
    case REC_route_update:
#ifdef DIST_IO_OPTIMIZATIONS 
      pair.record = SNetTlRead(pair.stream);
#endif /* DIST_IO_OPTIMIZATIONS */

      stream = SNetRecGetStream(pair.record);

      key = HASHTABLE_PTR_TO_KEY(stream);

      value_pair = CreatePair(SNetRecGetNode(pair.record),
			      SNetRecGetIndex(pair.record));

      SNetHashtablePut(routing_table, key, value_pair);

      set = SNetTlAddToStreamset(set, stream);

      SNetRecDestroy(pair.record);
      break;
#ifdef DIST_IO_OPTIMIZATIONS
    case REC_route_redirect:
      pair.record = SNetTlRead(pair.stream);

      index = SNetRecGetIndex(pair.record);
      node = SNetRecGetNode(pair.record);

      if(pair.stream == imanager) {

	if(node == -1 && index == -1) {
	  /* Cancelled */
	  redirected = NULL;

	} else {
	  key = SNetHashtableGetKey(routing_table, &index);
	  
	  value_pair = (pair_t *)SNetHashtableGet(routing_table, key);
	  
	  if(value_pair != NULL) {
#ifdef OMANAGER_DEBUG
	    debug_record_count++;
#endif /* OMANAGER_DEBUG */
 
	    node = value_pair->node;
	    
	    value_pair->node = SNetRecGetNode(pair.record);
	    
	    SNetRecSetNode(pair.record, my_rank);
	    
	    position = 0;
	    
	    result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);
	    
	    SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size);
	    
	    MPI_Send(buf, position, MPI_PACKED, node, SNET_msg_record, MPI_COMM_WORLD);

	  } 
	}
      } else {
#ifdef OMANAGER_DEBUG
	debug_record_count++;
#endif /* OMANAGER_DEBUG */
	key = HASHTABLE_PTR_TO_KEY(pair.stream);
	
	value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);
	
	set = SNetTlDeleteFromStreamset(set, pair.stream);

	/* No need to check for message size, as buf_size will always be greater! */
	
	position = 0;
	
	result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);
	
	SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size);
		
	MPI_Send(buf, position, MPI_PACKED, value_pair->node, SNET_msg_record, MPI_COMM_WORLD);
		
	DestroyPair(value_pair);
	
	redirected = NULL;
      }

      SNetRecDestroy(pair.record);
      break;
      case REC_route_concatenate:

	pair.record = SNetTlRead(pair.stream);

	stream = SNetRecGetStream(pair.record);

	set = SNetTlAddToStreamset(set, stream);	

	key = HASHTABLE_PTR_TO_KEY(stream);

	SNetHashtablePut(routing_table, key, CreatePair(my_rank, SNetRecGetIndex(pair.record)));

	SNetRecDestroy(pair.record);

	undo = true;

	break;
#endif /* DIST_IO_OPTIMIZATIONS */
      default:
      break;
    }
  }

  SNetMemFree(buf);

  SNetHashtableDestroy(routing_table);

  SNetTlDestroyStreamset(set);

#ifdef OMANAGER_DEBUG
  SNetUtilDebugNotice("Node %d: omanager sent %d records", my_rank, debug_record_count);
#endif /* OMANAGER_DEBUG */

  return NULL;
}



void OManagerCreate(snet_tl_stream_t *imanager)
{
  pthread_t thread;

  pthread_create(&thread, NULL, OManagerThread, (void *)imanager);
  pthread_detach(thread);

  return;
}
#endif /* DUMMY */
#endif /* DISTRIBUTED_SNET */
