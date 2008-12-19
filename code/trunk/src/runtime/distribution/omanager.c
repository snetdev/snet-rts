#ifdef DISTRIBUTED_SNET
#include <pthread.h>
#include <mpi.h>
#include <stdio.h>

#include "omanager.h"
#include "record.h"
#include "stream_layer.h"
#include "memfun.h"
#include "hashtable.h"
#include "message.h"

/* TODO: 
 *  - Route redirection
 *  - Route concatenation
 */

#define HASHTABLE_SIZE 32
#define ORIGINAL_MAX_MSG_SIZE 256

typedef struct {
  int node;
  int id;
} pair_t;
     
static int compare_fun(void *a, void *b) {
  pair_t *a2 = (pair_t *)a;
  pair_t *b2 = (pair_t *)b;

  return (a2->node == b2->node) && (a2->id == b2->id);
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

  routing_table = SNetHashtableCreate(HASHTABLE_SIZE, compare_fun);

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  buf_size = ORIGINAL_MAX_MSG_SIZE;
  buf = SNetMemAlloc(sizeof(char) * buf_size);

  set = SNetTlCreateStreamset();

  set = SNetTlAddToStreamset(set, imanager);

  while(!terminate) {

    pair = SNetTlReadMany(set);

    switch(SNetRecGetDescriptor(pair.record)) {
    case REC_sync:
      stream = SNetRecGetStream(pair.record);

      // TODO: Check if this is the output stream of imanager!

      key = HASHTABLE_PTR_TO_KEY(pair.stream);

      value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);

      key = HASHTABLE_PTR_TO_KEY(stream);

      SNetHashtablePut(routing_table, key, value_pair);    

      SNetTlReplaceInStreamset(set, pair.stream, stream);

      break;
    case REC_collect:
    case REC_sort_begin:	
    case REC_sort_end:
    case REC_probe:
    case REC_data: 
      key = HASHTABLE_PTR_TO_KEY(pair.stream);

      value_pair = (pair_t *)SNetHashtableGet(routing_table, key); 

      if(value_pair->node == my_rank) {
	// TODO: Fragmentation detected: concatenate streams!
      }

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

      break;
    case REC_terminate:
      if(pair.stream == imanager) {
	
	terminate = true;

      } else {
	key = HASHTABLE_PTR_TO_KEY(pair.stream);

	value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);

	/* No need to check for message size, as buf_size will always be greater! */

	position = 0;

	result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);

	SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size);

	MPI_Send(buf, position, MPI_PACKED, value_pair->node, SNET_msg_record, MPI_COMM_WORLD);
  
	DestroyPair(value_pair);
      }

      set = SNetTlDeleteFromStreamset(set, pair.stream);
      break;
    case REC_route_update:

      stream = SNetRecGetStream(pair.record);

      key = HASHTABLE_PTR_TO_KEY(stream);

      value_pair = CreatePair(SNetRecGetNode(pair.record),
			      SNetRecGetIndex(pair.record));

      SNetHashtablePut(routing_table, key, value_pair);

      set = SNetTlAddToStreamset(set, stream);

      break;
    case REC_route_redirect:
      if(pair.stream == imanager) {
	/* TODO: set route according to the record! */

      } else {
	
	key = HASHTABLE_PTR_TO_KEY(pair.stream);

	value_pair = (pair_t *)SNetHashtableRemove(routing_table, key);
	
	/* No need to check for message size, as buf_size will always be greater! */
	
	position = 0;

	result = MPI_Pack(&value_pair->id, 1, MPI_INT, buf, buf_size, &position, MPI_COMM_WORLD);

	SNetRecPack(pair.record, MPI_COMM_WORLD, &position, buf, buf_size);

	MPI_Send(buf, position, MPI_PACKED, value_pair->node, SNET_msg_record, MPI_COMM_WORLD);

	set = SNetTlDeleteFromStreamset(set, pair.stream);

	DestroyPair(value_pair);
	
	}
      break;
    default:
      break;
    }

    if(pair.stream != imanager 
       /* In this case the stream is already destroyed! */
       && SNetRecGetDescriptor(pair.record) != REC_terminate
       && SNetTlGetFlag(pair.stream)) {

      /* Empty stream! */
      
      /* TODO: Start redirection. */
    }

    SNetRecDestroy(pair.record);
  }

  SNetMemFree(buf);

  SNetHashtableDestroy(routing_table);

  SNetTlDestroyStreamset(set);

  return NULL;
}



void OManagerCreate(snet_tl_stream_t *imanager)
{
  pthread_t thread;

  pthread_create(&thread, NULL, OManagerThread, (void *)imanager);
  pthread_detach(thread);

  return;
}
#endif /* DISTRIBUTED_SNET */
