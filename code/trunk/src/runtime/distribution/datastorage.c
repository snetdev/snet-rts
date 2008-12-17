#ifdef DISTRIBUTED_SNET

#include <mpi.h>
#include <pthread.h>
#include <string.h>

#include "datastorage.h"
#include "memfun.h"
#include "hashtable.h"
#include "interface_functions.h"

#define HASHTABLE_SIZE 32

#define TAG_DATA_OP 0 

typedef enum {
  OP_copy, 
  OP_delete,
  OP_fetch,
  OP_terminate
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

void *DataManagerThread(void *ptr)
{
  MPI_Status status;
  MPI_Datatype datatype;
  MPI_Aint true_lb;
  MPI_Aint true_extent;
  bool terminate = false;
  int result;
  void *buf = NULL;
  int buf_size = 0;
  void *data;
  void *type_buf;
  int type_buf_size;
  int count;

  snet_msg_data_op_t *msg;
  snet_ref_t *ref;

  int interface;
  int size;

  result = MPI_Type_get_true_extent(storage.op_type, &true_lb, &true_extent);

  buf_size = true_extent;
  buf = SNetMemAlloc( sizeof(char) * buf_size);

  type_buf_size = 128;
  type_buf = SNetMemAlloc(sizeof(char) * type_buf_size);

  while(!terminate) {
    memset(buf, 0, buf_size);
      
    result = MPI_Recv(buf, 1, storage.op_type,  MPI_ANY_SOURCE, TAG_DATA_OP, storage.comm, &status);

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
      } 
  
      break;

    case OP_fetch:

      ref = SNetDataStorageGet(msg->id);

      if(ref != NULL) {
	/* TODO: Error handling! */

	interface = SNetRefGetInterface(ref);

	size = SNetGetSerializeTypeFun(interface)(storage.comm,
						  SNetRefGetData(ref),
						  type_buf, 
						  type_buf_size);


	MPI_Send(type_buf, size, MPI_PACKED, status.MPI_SOURCE, msg->op_id, storage.comm);

	data = SNetGetPackFun(interface)(storage.comm,
					 SNetRefGetData(ref), 
					 &datatype, 
					 &count);

	MPI_Send(data, count, datatype, status.MPI_SOURCE, msg->op_id, storage.comm);

	SNetGetCleanupFun(interface)(datatype,
				     data);

	SNetRefDestroy(ref);

      } else {

	printf("Fatal error: Unknown data referred in data fetch %lld\n", msg->id);
	abort();
      }
    
      break;

    case OP_terminate:
      terminate = true;
      
      break;
    }
    msg = NULL;
  }

  SNetMemFree(buf);
  SNetMemFree(type_buf);

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
  
  buf = SNetMemAlloc(sizeof(char) * true_extent * count);  
  
  result = MPI_Recv(buf, count, MPI_PACKED, location, 
		    msg.op_id, storage.comm, &status);
  
  type = SNetGetDeserializeTypeFun(interface)(storage.comm, buf, count);
  
  SNetMemFree(buf);
  
  result = MPI_Probe(location, msg.op_id, storage.comm, &status);
  
  result = MPI_Get_count(&status, type, &count);
  
  result = MPI_Type_get_true_extent(type, &true_lb, &true_extent);
  
  
  buf = SNetMemAlloc(sizeof(char) * true_extent * count);  
  
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
