#ifdef DISTRIBUTED_SNET

#include <mpi.h>
#include <pthread.h>
#include <string.h>

#include "datastorage.h"
#include "bitmap.h"
#include "memfun.h"
#include "hashtable.h"
#include "interface_functions.h"

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
  snet_hashtable_t *hashtable;
  unsigned int op_id;
  pthread_t thread;
} storage_t;

static storage_t storage;

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
    ops[bit].ref = NULL;
    ops[bit].buf_size = 128;
    ops[bit].buf = SNetMemAlloc(sizeof(char) * ops[bit].buf_size);
    ops[bit].interface = 0;
    ops[bit].data = NULL;;
    ops[bit].type = MPI_DATATYPE_NULL;
  }

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
	  printf("Remote copy failed %lld\n", msg->id);
	  abort();
	}
	
	break;
	
      case OP_delete:    
	
	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {
	  SNetRefDestroy(ref);
	} else {
	  printf("Remote destroy failed %lld\n", msg->id);
	  abort();
	}
	
	break;
	
      case OP_fetch:
	
	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {
	  
	  if((bit = SNetUtilBitmapFindNSet(map)) == SNET_BITMAP_ERR) {
	    result = MPI_Waitany(MAX_REQUESTS, requests, &bit, &req_status);
	    SNetUtilBitmapSet(map, bit);
	  }
	  
	  ops[bit].ref = ref;
	  ops[bit].dest = status.MPI_SOURCE;
	  ops[bit].id = msg->op_id;
	  
	  ops[bit].interface = SNetRefGetInterface(ops[bit].ref);
	  
	  size = SNetGetSerializeTypeFun(ops[bit].interface)(storage.comm,
							     SNetRefGetData(ops[bit].ref),
							     ops[bit].buf, 
							     ops[bit].buf_size);
	  
	  result = MPI_Isend(ops[bit].buf, size, MPI_PACKED, ops[bit].dest, 
			     ops[bit].id, storage.comm, &requests[bit]);
	  
	  ops[bit].op = OP_type;
	  
	} else {
	  
	  printf("Fatal error: Unknown data referred in data fetch %lld\n", msg->id);
	  abort();
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

  /* TODO: Handle pending requests! */

  SNetUtilBitmapDestroy(map);
  SNetMemFree(buf);

  for(bit = 0; bit < MAX_REQUESTS; bit++) {
    SNetMemFree(ops[bit].buf);
  }

  return NULL;
}

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

  MPI_Comm_dup(MPI_COMM_WORLD, &storage.comm);

  MPI_Comm_rank(storage.comm, &storage.self);

  pthread_mutex_init(&storage.mutex, NULL);

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
    
    SNetRefDestroy(ref);
    
    ref = temp;
  }

  pthread_mutex_unlock(&storage.mutex);

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

void SNetDataStorageRemove(snet_id_t id)
{
  pthread_mutex_lock(&storage.mutex);

  SNetHashtableRemove(storage.hashtable, id);

  pthread_mutex_unlock(&storage.mutex);
}

void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location)
{
  snet_msg_data_op_t msg;
  MPI_Status status;
  MPI_Datatype type;
  int result;
  void *buf;
  void *data;
  int count;
  MPI_Aint true_lb;
  MPI_Aint true_extent;

  pthread_mutex_lock(&storage.mutex);
    
  msg.op_id = storage.op_id++;
  
  if(msg.op_id == TAG_DATA_OP) {
    msg.op_id = TAG_DATA_OP + 1;
  }
  
  pthread_mutex_unlock(&storage.mutex);
    
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

  msg.op = OP_copy;
  msg.op_id = 0;
  msg.id = id;

  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);
}

void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location) 
{
  snet_msg_data_op_t msg;

  msg.op = OP_delete;
  msg.op_id = 0;
  msg.id = id;

  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);
}

#endif /* DISTRIBUTED_SNET */
