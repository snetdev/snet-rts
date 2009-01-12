#ifdef DISTRIBUTED_SNET

#include <mpi.h>
#include <pthread.h>
#include <string.h>

#include "datastorage.h"
#include "bitmap.h"
#include "memfun.h"
#include "hashtable.h"
#include "interface_functions.h"
#include "debug.h"

#define HASHTABLE_SIZE 32
#define MAX_REQUESTS 32

#define TAG_DATA_OP 0 

typedef enum {
  OP_copy, 
  OP_delete,
  OP_fetch,
  OP_terminate,
  OP_type,
  OP_data
} snet_data_op_t;

typedef struct snet_msg_data_op {
  snet_data_op_t op;
  int op_id;
  snet_id_t id;
} snet_msg_data_op_t;

typedef struct storage {
  int self;
  MPI_Comm comm;
  MPI_Datatype op_type;
  pthread_mutex_t mutex;
  pthread_mutex_t id_mutex;
  snet_hashtable_t *hashtable;
  unsigned int op_id;
  pthread_t thread;
} storage_t;

static storage_t storage;

#define CONCURRENT_DATA_OPERATIONS

#ifdef CONCURRENT_DATA_OPERATIONS
typedef struct snet_fetch_request {
  snet_data_op_t op;
  int id;
  snet_ref_t *ref;
  void *buf;
  int buf_size;
  int interface;
  void *data;
  int dest;
  MPI_Datatype type;
} snet_fetch_request_t;

void *DataManagerThread(void *ptr)
{
  MPI_Status status;

  MPI_Aint true_lb;
  MPI_Aint true_extent;

  bool terminate = false;
  int result;
  void *buf = NULL;
  int buf_size = 0;
 
  int count;

  snet_msg_data_op_t *msg;
  snet_ref_t *ref;

  int size;

  MPI_Status req_status;

  MPI_Request requests[MAX_REQUESTS + 1];
  snet_fetch_request_t ops[MAX_REQUESTS];

  snet_util_bitmap_t *map;
  int bit;
  bool recv;

  map = SNetUtilBitmapCreate(MAX_REQUESTS);

  result = MPI_Type_get_true_extent(storage.op_type, &true_lb, &true_extent);

  buf_size = true_extent;
  buf = SNetMemAlloc(buf_size);

  for(bit = 0; bit < MAX_REQUESTS; bit++) {
    requests[bit] = MPI_REQUEST_NULL;
    ops[bit].op = OP_type;
    ops[bit].id = 0;
    ops[bit].ref = NULL;
    ops[bit].buf_size = 128;
    ops[bit].buf = SNetMemAlloc(sizeof(char) * ops[bit].buf_size);
    ops[bit].interface = 0;
    ops[bit].data = NULL;
    ops[bit].type = MPI_DATATYPE_NULL;
  }

  requests[MAX_REQUESTS] = MPI_REQUEST_NULL;

  recv = true;

  while(!terminate) {
  
    if(recv) {
      memset(buf, 0, buf_size);
      result = MPI_Irecv(buf, 1, storage.op_type, MPI_ANY_SOURCE, TAG_DATA_OP, 
			 storage.comm, &requests[MAX_REQUESTS]);
      recv = false;
    }

    result = MPI_Waitany(MAX_REQUESTS + 1, requests, &bit, &status);

    if(bit == MAX_REQUESTS) {
      recv = true;
      msg = (snet_msg_data_op_t *)buf;

      switch(msg->op) {
      case OP_copy:
	
	ref = SNetDataStorageCopy(msg->id);
	
	if(ref == NULL) {
	SNetUtilDebugFatal("Node %d: Remote copy from %d failed (%d:%d)\n",
			   storage.self, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
	} else {

	  /* TODO: this could be non-blocking */
	  result = MPI_Send(&msg, 0, MPI_INT, status.MPI_SOURCE, 
			    msg->op_id, storage.comm);

	}

	break;
	
      case OP_delete:  
	
	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {

	  SNetRefDestroy(ref);

	} else {
	  SNetUtilDebugFatal("Node %d: Remote delete from %d failed (%d:%d)\n",
			     storage.self, 
			     status.MPI_SOURCE, 
			     SNET_ID_GET_NODE(msg->id), 
			     SNET_ID_GET_ID(msg->id));
	}
	
	break;
	
      case OP_fetch:
	
	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {
	  
	  if((bit = SNetUtilBitmapFindNSet(map)) == SNET_BITMAP_ERR) {
	    result = MPI_Waitany(MAX_REQUESTS, requests, &bit, &req_status);

	    /* TODO: Handle the completed call! */

	    SNetUtilDebugFatal("Max number of concurrent remote data operations: Waiting feature not yet implemented!");
	  
	    SNetUtilBitmapSet(map, bit);
	  }
	  
	  ops[bit].ref = ref;
	  ops[bit].dest = status.MPI_SOURCE;
	  ops[bit].id = msg->op_id;
	  
	  ops[bit].interface = SNetRefGetInterface(ops[bit].ref);

	  size = SNetGetSerializeTypeFun(ops[bit].interface)(storage.comm, SNetRefGetData(ops[bit].ref), ops[bit].buf, ops[bit].buf_size);

	  result = MPI_Isend(ops[bit].buf, size, MPI_PACKED, ops[bit].dest, 
			     ops[bit].id, storage.comm, &requests[bit]);
	  
	  ops[bit].op = OP_type;
  
	} else {
	  SNetUtilDebugFatal("Node %d: Remote fetch from %d failed (%d:%d)\n",
			     storage.self, 
			     status.MPI_SOURCE, 
			     SNET_ID_GET_NODE(msg->id), 
			     SNET_ID_GET_ID(msg->id));
	}
	
	break;
	
      case OP_terminate:
	terminate = true;
	
	break;
      default:
	break;
      }
    } else {
      switch(ops[bit].op) {
	
      case OP_type:
	
	ops[bit].data = SNetGetPackFun(ops[bit].interface)(storage.comm,
							   SNetRefGetData(ops[bit].ref), 
							   &ops[bit].type, 
							   &count);

	result = MPI_Isend(ops[bit].data, count, ops[bit].type, ops[bit].dest, 
			   ops[bit].id, storage.comm, &requests[bit]);
	
	ops[bit].op = OP_data;
	break;
      case OP_data:
	SNetGetCleanupFun(ops[bit].interface)(ops[bit].type, ops[bit].data);
	
	SNetRefDestroy(ops[bit].ref);
	
	SNetUtilBitmapClear(map, bit);
	break;
      default:
	break;
      }
    }
    
    msg = NULL;
  }

  SNetUtilBitmapDestroy(map);
  SNetMemFree(buf);

  for(bit = 0; bit < MAX_REQUESTS; bit++) {
    SNetMemFree(ops[bit].buf);
  }

  return NULL;
}

#else /* CONCURRENT_DATA_OPERATIONS*/

void *DataManagerThread(void *ptr)
{
  MPI_Status status;

  MPI_Datatype type;

  MPI_Aint true_lb;
  MPI_Aint true_extent;

  bool terminate = false;
  int result;
  void *buf = NULL;
  int buf_size = 0;
 
  int count;

  snet_msg_data_op_t *msg;
  snet_ref_t *ref;

  int interface;

  snet_id_t id;
  int op_id;

  void *data;

  int size;

  result = MPI_Type_get_true_extent(storage.op_type, &true_lb, &true_extent);

  buf_size = true_extent;
  buf = SNetMemAlloc(buf_size);

  type = MPI_DATATYPE_NULL;

  while(!terminate) {
    
    memset(buf, 0, buf_size);

    result = MPI_Recv(buf, 1, storage.op_type, MPI_ANY_SOURCE, TAG_DATA_OP, 
		      storage.comm, &status);
 
    msg = (snet_msg_data_op_t *)buf;

    switch(msg->op) {
    case OP_copy:

      ref = SNetDataStorageCopy(msg->id);
      
      if(ref == NULL) {
	SNetUtilDebugFatal("Node %d: Remote copy from %d failed (%d:%d)\n",
			   storage.self, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      } else {

	result = MPI_Send(&msg, 0, MPI_INT, status.MPI_SOURCE, 
			  msg->op_id, storage.comm);
      }
      
      break;
      
    case OP_delete:    

      ref = SNetDataStorageGet(msg->id);
	
      if(ref != NULL) {
	SNetRefDestroy(ref);
      } else {
	SNetUtilDebugFatal("Node %d: Remote delete from %d failed (%d:%d)\n",
			   storage.self, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      }
      
      break;
      
    case OP_fetch:
      
      ref = SNetDataStorageGet(msg->id);

      id = msg->id;
      op_id =msg->op_id;
      
      if(ref != NULL) {

	interface = SNetRefGetInterface(ref);
	
	size = SNetGetSerializeTypeFun(interface)(storage.comm, SNetRefGetData(ref), buf, buf_size);

	result = MPI_Ssend(buf, size, MPI_PACKED, status.MPI_SOURCE, 
			   op_id, storage.comm);
       
	data = SNetGetPackFun(interface)(storage.comm,
					 SNetRefGetData(ref), 
					 &type, 
					 &count);

	result = MPI_Ssend(data, count, type, status.MPI_SOURCE, 
			   op_id, storage.comm);
	
	SNetGetCleanupFun(interface)(type, data);
	
	SNetRefDestroy(ref);

      } else {
	SNetUtilDebugFatal("Node %d: Remote fetch from %d failed (%d:%d)\n",
			   storage.self, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      }
      break;
      
    case OP_terminate:
      terminate = true;
      
      break;
    default:
      break;
    }
    
    msg = NULL;
  }

  SNetMemFree(buf);

  return NULL;
}
#endif /* CONCURRENT_DATA_OPERATIONS*/

static void DataManagerInit()
{
  pthread_create(&storage.thread, NULL, DataManagerThread, NULL);
}

static void DataManagerDestroy()
{
  snet_msg_data_op_t msg;

  msg.op = OP_terminate;
  msg.op_id = 0;
  msg.id = 0;

  MPI_Send(&msg, 1, storage.op_type, storage.self, TAG_DATA_OP, storage.comm);

  pthread_join(storage.thread, NULL);
}

void SNetDataStorageInit() 
{
  int block_lengths[2];
  MPI_Aint displacements[2];
  MPI_Datatype types[2];
  MPI_Aint base;
  snet_msg_data_op_t msg;
  int i;
  int result;

  result = MPI_Comm_dup(MPI_COMM_WORLD, &storage.comm);

  result = MPI_Comm_rank(storage.comm, &storage.self);

  pthread_mutex_init(&storage.mutex, NULL);

  pthread_mutex_init(&storage.id_mutex, NULL);

  storage.hashtable = SNetHashtableCreate(HASHTABLE_SIZE, NULL);

  storage.op_id = TAG_DATA_OP + 1;


  /* MPI_Datatype for TAG_DATA_OP */

  types[0] = MPI_INT;
  types[1] = SNET_ID_MPI_TYPE;
 
  block_lengths[0] = 2;
  block_lengths[1] = 1;
   
  MPI_Address(&msg.op, displacements);
  MPI_Address(&msg.id, displacements + 1);
  
  base = displacements[0];
  
  for(i = 0; i < 2; i++) {
    displacements[i] -= base;
  }

  MPI_Type_create_struct(2, block_lengths, displacements,
			 types, &storage.op_type);

  MPI_Type_commit(&storage.op_type);



  DataManagerInit();
}

void SNetDataStorageDestroy()
{
  DataManagerDestroy();

  pthread_mutex_destroy(&storage.mutex);

  pthread_mutex_destroy(&storage.id_mutex);

  MPI_Type_free(&storage.op_type);

  MPI_Comm_free(&storage.comm);

  SNetHashtableDestroy(storage.hashtable);
}

snet_ref_t *SNetDataStoragePut(snet_id_t id, snet_ref_t *ref)
{
  snet_ref_t *temp;

  pthread_mutex_lock(&storage.mutex);
  
  if(SNetHashtablePut(storage.hashtable, id, ref) != 0){

    temp = SNetHashtableGet(storage.hashtable, id);  

    temp = SNetRefCopy(temp);

    pthread_mutex_unlock(&storage.mutex);
    
    SNetRefDestroy(ref);
    
    ref = temp;
  } else {
    pthread_mutex_unlock(&storage.mutex);
  }

  return ref;
}

snet_ref_t *SNetDataStorageCopy(snet_id_t id)
{
  snet_ref_t *temp;

  pthread_mutex_lock(&storage.mutex);
  
  temp = SNetHashtableGet(storage.hashtable, id);
    
  if(temp != NULL) {    
    temp = SNetRefCopy(temp);
  }

  pthread_mutex_unlock(&storage.mutex);

  return temp;
}

snet_ref_t *SNetDataStorageGet(snet_id_t id)
{
  snet_ref_t *ref;

  pthread_mutex_lock(&storage.mutex);
  
  ref = SNetHashtableGet(storage.hashtable, id);

  pthread_mutex_unlock(&storage.mutex);

  return ref;
}

bool SNetDataStorageRemove(snet_id_t id)
{
  bool ret = false;
  snet_ref_t *ref;

  pthread_mutex_lock(&storage.mutex);

  ref = SNetHashtableGet(storage.hashtable, id);

  if(SNetRefGetRefCount(ref) == 1) {

    SNetHashtableRemove(storage.hashtable, id);

    ret = true;
  }

  pthread_mutex_unlock(&storage.mutex);

  return ret;
}

void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location)
{
  snet_msg_data_op_t msg;
  MPI_Status status;
  MPI_Datatype type;
  int result = 0;
  void *buf = NULL;
  void *data = NULL;
  int count = 0;
  MPI_Aint true_lb;
  MPI_Aint true_extent;

  pthread_mutex_lock(&storage.id_mutex);
    
  msg.op_id = storage.op_id++;
  
  if(msg.op_id == TAG_DATA_OP) {
    msg.op_id = TAG_DATA_OP + 1;
  }
  
  pthread_mutex_unlock(&storage.id_mutex);
    
  msg.op = OP_fetch;
  msg.id = id;

  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);
  
  result = MPI_Probe(location, msg.op_id, storage.comm, &status);
  
  result = MPI_Get_count(&status, MPI_PACKED, &count);
  
  result = MPI_Type_get_true_extent(MPI_PACKED, &true_lb, &true_extent);
  
  buf = SNetMemAlloc(true_extent * count);  
  
  result = MPI_Recv(buf, count, MPI_PACKED, location, 
		    msg.op_id, storage.comm, &status);
  
  type = SNetGetDeserializeTypeFun(interface)(storage.comm, buf, count);
  
  SNetMemFree(buf);

  result = MPI_Probe(location, msg.op_id, storage.comm, &status);
  
  result = MPI_Get_count(&status, type, &count);
  
  result = MPI_Type_get_true_extent(type, &true_lb, &true_extent);
  
  
  buf = SNetMemAlloc(true_extent * count);  

  result = MPI_Recv(buf, count, type, location, 
		    msg.op_id, storage.comm, &status);

  data = SNetGetUnpackFun(interface)(storage.comm, buf, type, count);
  
  SNetGetCleanupFun(interface)(type, buf);

  return data;
}

  void SNetDataStorageRemoteCopy(snet_id_t id, unsigned int location)
{
  snet_msg_data_op_t msg;
  MPI_Status status;

  msg.op = OP_copy;
  msg.op_id = 0;
  msg.id = id;

  pthread_mutex_lock(&storage.id_mutex);
    
  msg.op_id = storage.op_id++;
  
  if(msg.op_id == TAG_DATA_OP) {
    msg.op_id = TAG_DATA_OP + 1;
  }
  
  pthread_mutex_unlock(&storage.id_mutex);
  
  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);

  MPI_Recv(&msg, 0, MPI_INT, location, msg.op_id, storage.comm, &status);

}

void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location) 
{
  snet_msg_data_op_t msg;

  msg.op = OP_delete;
  msg.op_id = 0;
  msg.id = id;

  pthread_mutex_lock(&storage.id_mutex);
    
  pthread_mutex_unlock(&storage.id_mutex);
  
  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);
}

#endif /* DISTRIBUTED_SNET */
