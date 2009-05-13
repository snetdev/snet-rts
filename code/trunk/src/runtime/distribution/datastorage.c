#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file datastorage.c
 *
 * @brief Implements datastorage system used in distributed S-Net.
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 16.1.2009
 *
 *****************************************************************************/

#include <mpi.h>
#include <pthread.h>
#include <string.h>

#include "datastorage.h"
#include "bitmap.h"
#include "memfun.h"
#include "hashtable.h"
#include "interface_functions.h"
#include "debug.h"

#ifdef SNET_DEBUG_COUNTERS
#include "debugtime.h"
#include "debugcounters.h"
#endif /* SNET_DEBUG_COUNTERS  */

/** <!--********************************************************************-->
 *
 * @name Datastorage
 *
 * <!--
 * static void *DataManagerThread(void *ptr) : Main loop of the data manager.
 * static void DataManagerInit() : Starts the data manager.
 * static void DataManagerDestroy() : Stops the data manager.
 * void SNetDataStorageInit() : Initializes the data storage.
 * void SNetDataStorageDestroy() : Destroys the data storage.
 * snet_ref_t *SNetDataStoragePut(snet_id_t id, snet_ref_t *ref) : Adds new reference into the data storage.
 * snet_ref_t *SNetDataStorageCopy(snet_id_t id) : Copies a reference.
 * snet_ref_t *SNetDataStorageGet(snet_id_t id) : Returns a reference.
 * bool SNetDataStorageRemove(snet_id_t id) : Removes a reference from the data storage.
 * void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location) : Fetches data from remote node.
 * void SNetDataStorageRemoteCopy(snet_id_t id, unsigned int location) : Copies data on remote node.
 * void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location) : Deletes data from remote node.
 * -->
 *
 *****************************************************************************/
/*@{*/

/* Decideds if the data manager can serve concurrent fetches. */
#define CONCURRENT_DATA_OPERATIONS

#ifdef CONCURRENT_DATA_OPERATIONS
/* Maximum number of concurrent fetches alloed. */
#define MAX_REQUESTS 32
#endif /* CONCURRENT_DATA_OPERATIONS */

/* Number of chains in data storage hashtable. 
 * This value should be a prime number!
 */
#define HASHTABLE_SIZE 31

/* MPI TAG for data operations. */
#define TAG_DATA_OP 0 


/** enum snet_data_op_t
 *
 *   @brief Types for remote data operations.
 *
 */

typedef enum {
  OP_copy,       /**< Remote copy. */
  OP_delete,     /**< Remote delete. */
  OP_fetch,      /**< Remote fetch. */
#ifdef CONCURRENT_DATA_OPERATIONS
  OP_type,       /**< Phase of fetch: type sent. */
  OP_data,       /**< Phase of fetch: data sent. */
#endif /* CONCURRENT_DATA_OPERATIONS */
  OP_terminate   /**< Terminate system. */
} snet_data_op_t;



/** @struct snet_msg_data_op_t
 *
 *   @brief Data operation message.
 *
 */

typedef struct snet_msg_data_op {
  snet_data_op_t op; /**< Operation type. */
  int op_id;         /**< Operation id. */
  snet_id_t id;      /**< Data id. */
} snet_msg_data_op_t;


/** @struct storage_t
 *
 *   @brief Structure to hold the data storage.
 *
 *   This data structure holds common values shared by the
 *   data manager and calls of data storage functions by other threads.
 *
 */

typedef struct storage {
  MPI_Comm comm;                /**< MPI Communicator used for data messges. */
  int rank;                     /**< Rank of the node in 'comm'. */
  MPI_Datatype op_type;         /**< Committed MPI type for 'snet_msg_data_op_t'. */
  snet_hashtable_t *hashtable;  /**< The data storage. */
  pthread_mutex_t mutex;        /**< Mutex to guard access to 'hashtable'. */
  unsigned int op_id;           /**< Next free operation ID. */
  pthread_mutex_t id_mutex;     /**< Mutex to guard access to 'op_id'. */
  pthread_t thread;             /**< Data manager thread (needed for join). */
} storage_t;

static storage_t storage;       /**< Variable to hold the common data. */


#ifdef CONCURRENT_DATA_OPERATIONS

/** @struct snet_fetch_request_t
 *
 *   @brief Hold data of single remote fetch operation.
 *
 */

typedef struct snet_fetch_request {
  snet_data_op_t op; /**< The (next) operation. */
  int id;            /**< operation ID. */
  snet_ref_t *ref;   /**< Reference related to the operation. */
  void *buf;         /**< Buffer used to send data. */
  int buf_size;      /**< Size of the buffer. */
  int interface;     /**< Langauge interface of the data. */
  void *opt;         /**< Optional return value. */
  int dest;          /**< Destionation node (rank). */
  MPI_Datatype type; /**< MPI type of the data. */
} snet_fetch_request_t;

/** <!--********************************************************************-->
 *
 * @fn  static void *DataManagerThread(void *ptr)
 *
 *   @brief  Main loop for the data manager.
 *
 *           Waits for remote operation requests and serves them. 
 *
 *   @param ptr   NULL.
 *
 *   @return      NULL.
 *
 ******************************************************************************/

static void *DataManagerThread(void *ptr)
{
  MPI_Status status;
  MPI_Aint true_lb;
  MPI_Aint true_extent;
  MPI_Status req_status;

  bool terminate = false;
  int result;

  void *buf = NULL;
  int buf_size = 0;
  int count;
  void *data;

  snet_msg_data_op_t *msg;
  snet_ref_t *ref;
  int size;

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
    ops[bit].opt = NULL;
    ops[bit].type = MPI_DATATYPE_NULL;
  }

  requests[MAX_REQUESTS] = MPI_REQUEST_NULL;

  recv = true;


  while(!terminate) {
  
    if(recv) {
      /* Receive new operation: */
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
	/* Remote copy */

	ref = SNetDataStorageCopy(msg->id);
	
	if(ref == NULL) {
	  SNetUtilDebugFatal("Node %d: Remote copy from %d failed (%d:%d)\n",
			     storage.rank, 
			     status.MPI_SOURCE, 
			     SNET_ID_GET_NODE(msg->id), 
			     SNET_ID_GET_ID(msg->id));
	} else {

	  /* TODO: this could be non-blocking too */
	  result = MPI_Send(&msg, 0, MPI_INT, status.MPI_SOURCE, 
			    msg->op_id, storage.comm);

	}

	break;	
      case OP_delete:  
	/* Remote delete */

	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {

	  SNetRefDestroy(ref);

	} else {
	  SNetUtilDebugFatal("Node %d: Remote delete from %d failed (%d:%d)\n",
			     storage.rank, 
			     status.MPI_SOURCE, 
			     SNET_ID_GET_NODE(msg->id), 
			     SNET_ID_GET_ID(msg->id));
	}
	
	break;	
      case OP_fetch:
	/* Remote fetch, part 1 - Send type */
	
	ref = SNetDataStorageGet(msg->id);
	
	if(ref != NULL) {
	  
	  /* Acquire free index for the operation. */
	  while((bit = SNetUtilBitmapFindNSet(map)) == SNET_BITMAP_ERR) {
	    /* No free indices, wait until a fetch operation ends */

	    result = MPI_Waitany(MAX_REQUESTS, requests, &bit, &req_status);

	    switch(ops[bit].op) {
	
	    case OP_type:
	      /* Remote fetch, part 2 - Send data */
	      
	      count = SNetGetPackFun(ops[bit].interface)(storage.comm,
							 SNetRefGetData(ops[bit].ref), 
							 &ops[bit].type, 
							 &data,
							 &ops[bit].opt);
	      
	      result = MPI_Isend(data, count, ops[bit].type, ops[bit].dest, 
				 ops[bit].id, storage.comm, &requests[bit]);
	      
	      
	      ops[bit].op = OP_data;
	      break;
	    case OP_data:
	      /* Remote fetch, part 3 - Cleanup*/
	      SNetGetCleanupFun(ops[bit].interface)(ops[bit].type, ops[bit].opt);
	      
	      SNetRefDestroy(ops[bit].ref);
	      
	      SNetUtilBitmapClear(map, bit);
	      break;
	    default:
	      break;
	    }
	  }

	  SNetUtilBitmapSet(map, bit);
	  

	  ops[bit].ref = ref;
	  ops[bit].dest = status.MPI_SOURCE;
	  ops[bit].id = msg->op_id;
	  
	  ops[bit].interface = SNetRefGetInterface(ops[bit].ref);

	  /* Send type: */

	  size = SNetGetSerializeTypeFun(ops[bit].interface)(storage.comm, SNetRefGetData(ops[bit].ref), ops[bit].buf, ops[bit].buf_size);

	  result = MPI_Isend(ops[bit].buf, size, MPI_PACKED, ops[bit].dest, 
			     ops[bit].id, storage.comm, &requests[bit]);
	  
	  /* Next phase: */
	  ops[bit].op = OP_type;
  
	} else {
	  SNetUtilDebugFatal("Node %d: Remote fetch from %d failed (%d:%d)\n",
			     storage.rank, 
			     status.MPI_SOURCE, 
			     SNET_ID_GET_NODE(msg->id), 
			     SNET_ID_GET_ID(msg->id));
	}
	
	break;	
      case OP_terminate:
	/* Teminate */
	terminate = true;	
	break;
      default:
	break;
      }
    } else {
      switch(ops[bit].op) {
	
      case OP_type:
	/* Remote fetch, part 2 - Send data */

	count = SNetGetPackFun(ops[bit].interface)(storage.comm,
						   SNetRefGetData(ops[bit].ref), 
						   &ops[bit].type, 
						   &data,
						   &ops[bit].opt);

	result = MPI_Isend(data, count, ops[bit].type, ops[bit].dest, 
			   ops[bit].id, storage.comm, &requests[bit]);
	

	ops[bit].op = OP_data;
	break;
      case OP_data:
	/* Remote fetch, part 3 - Cleanup*/
	SNetGetCleanupFun(ops[bit].interface)(ops[bit].type, ops[bit].opt);
	
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

/** <!--********************************************************************-->
 *
 * @fn  static void *DataManagerThread(void *ptr)
 *
 *   @brief  Main loop for the data manager.
 *
 *           Waits for remote operation requests and serves them.
 *
 *   @param ptr   NULL.
 *
 *   @return      NULL.
 *
 ******************************************************************************/

static void *DataManagerThread(void *ptr)
{
  MPI_Status status;
  MPI_Datatype type = MPI_DATATYPE_NULL;
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
  void *opt;

  result = MPI_Type_get_true_extent(storage.op_type, &true_lb, &true_extent);

  buf_size = true_extent;

  buf = SNetMemAlloc(buf_size);


  while(!terminate) {
    
    memset(buf, 0, buf_size);

    result = MPI_Recv(buf, 1, storage.op_type, MPI_ANY_SOURCE, TAG_DATA_OP, 
		      storage.comm, &status);
 
    msg = (snet_msg_data_op_t *)buf;

    switch(msg->op) {
    case OP_copy:
      /* Remote copy */

      ref = SNetDataStorageCopy(msg->id);
      
      if(ref == NULL) {
	SNetUtilDebugFatal("Node %d: Remote copy from %d failed (%d:%d)\n",
			   storage.rank, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      } else {

	result = MPI_Send(&msg, 0, MPI_INT, status.MPI_SOURCE, 
			  msg->op_id, storage.comm);
      }
      
      break;
      
    case OP_delete:    

      /* Remote delete */

      ref = SNetDataStorageGet(msg->id);
	
      if(ref != NULL) {
	SNetRefDestroy(ref);
      } else {
	SNetUtilDebugFatal("Node %d: Remote delete from %d failed (%d:%d)\n",
			   storage.rank, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      }
      
      break;     
    case OP_fetch:
      /* Remote fetch */

      ref = SNetDataStorageGet(msg->id);

      id = msg->id;
      op_id =msg->op_id;
      
      if(ref != NULL) {

	interface = SNetRefGetInterface(ref);

	/* Send type: */
	
	size = SNetGetSerializeTypeFun(interface)(storage.comm, SNetRefGetData(ref), buf, buf_size);

	result = MPI_Send(buf, size, MPI_PACKED, status.MPI_SOURCE, 
			   op_id, storage.comm);
       
	/* Send data: */

	count = SNetGetPackFun(interface)(storage.comm,
					  SNetRefGetData(ref), 
					  &type,
					  &data,
					  &opt);

	result = MPI_Send(data, count, type, status.MPI_SOURCE, 
			  op_id, storage.comm);
	
	SNetGetCleanupFun(interface)(type, opt);

	/* Decrease ref count by one: */

	SNetRefDestroy(ref);

      } else {
	SNetUtilDebugFatal("Node %d: Remote fetch from %d failed (%d:%d)\n",
			   storage.rank, 
			   status.MPI_SOURCE, 
			   SNET_ID_GET_NODE(msg->id), 
			   SNET_ID_GET_ID(msg->id));
      }
      break;      
    case OP_terminate:
      /* Teminate */
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


/** <!--********************************************************************-->
 *
 * @fn  static void DataManagerInit()
 *
 *   @brief Starts the data manager.
 *
 ******************************************************************************/

static void DataManagerInit()
{
  pthread_create(&storage.thread, NULL, DataManagerThread, NULL);
}


/** <!--********************************************************************-->
 *
 * @fn  static void DataManagerDestroy()
 *
 *   @brief Stops the data manager.
 *
 *          The call will block until the data manager has stopped.
 *
 ******************************************************************************/

static void DataManagerDestroy()
{
  snet_msg_data_op_t msg;

  msg.op = OP_terminate;
  msg.op_id = 0;
  msg.id = 0;

  MPI_Send(&msg, 1, storage.op_type, storage.rank, TAG_DATA_OP, storage.comm);

  pthread_join(storage.thread, NULL);
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetDataStorageInit()
 *
 *   @brief Initializes the data storage
 *
 *          Initializes data storage and starts the data manager.
 *
 ******************************************************************************/
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

  result = MPI_Comm_rank(storage.comm, &storage.rank);

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

/** <!--********************************************************************-->
 *
 * @fn  void SNetDataStorageDestroy()
 *
 *   @brief Destroys the data storage
 *
 *          Destroys data storage and stops the data manager.
 *
 ******************************************************************************/

void SNetDataStorageDestroy()
{
  DataManagerDestroy();

  pthread_mutex_destroy(&storage.mutex);

  pthread_mutex_destroy(&storage.id_mutex);

  MPI_Type_free(&storage.op_type);

  MPI_Comm_free(&storage.comm);

  SNetHashtableDestroy(storage.hashtable);
}

/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetDataStoragePut(snet_id_t id, snet_ref_t *ref)
 *
 *   @brief  Inserts reference 'ref' to the data storage with ID 'id'.
 *
 *           If the ID is already in use, the reference behind the ID is copied
 *           instead and this reference is returned. The reference 'ref' is 
 *           automatically destroyed in this case.
 *
 *   @param ref  The reference to insert.
 *   @param id   The ID of the reference.
 *
 *   @return     The reference 'ref', or another reference by the ID 'id',
 *               if the ID was already used.
 *
 ******************************************************************************/

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


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetDataStorageCopy(snet_id_t id)
 *
 *   @brief  Copies the reference with ID 'id'.
 *
 *
 *   @param id  The ID of the reference to copy.
 *
 *   @return    The copied reference, or NULL in case no reference was found with 'id'.
 *
 ******************************************************************************/

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


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetDataStorageGet(snet_id_t id)
 *
 *   @brief  Returns the reference with ID 'id'.
 *
 *
 *   @param id  The ID of the reference to return.
 *
 *   @return    The reference, or NULL in case no reference was found with 'id'.
 *
 ******************************************************************************/

snet_ref_t *SNetDataStorageGet(snet_id_t id)
{
  snet_ref_t *ref;

  pthread_mutex_lock(&storage.mutex);
  
  ref = SNetHashtableGet(storage.hashtable, id);

  pthread_mutex_unlock(&storage.mutex);

  return ref;
}


/** <!--********************************************************************-->
 *
 * @fn  bool SNetDataStorageRemove(snet_id_t id)
 *
 *   @brief  Removes the reference with ID 'id' from the data storage.
 *
 *           The reference is not removed if reference count is more than 1.
 *
 *
 *   @param id  The ID of the reference to remove.
 *
 *   @return    'true' if the reference was really removed, 'false' otherwise.
 *
 ******************************************************************************/

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


/** <!--********************************************************************-->
 *
 * @fn  void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location)
 *
 *   @brief  Fetches data from remote node.
 *
 *           Blocks until the data is fetched.
 *
 *   @note   Causes multiple remote operations. 
 *   @note   Safe to use inside a locked reference!
 *
 *   @param id         The ID of the data to fetch.
 *   @param interface  The language interface of teh data.
 *   @param location   Current location of the data.
 *
 *   @return           Pointer to the fetched data, or NULL in case of failure.
 *
 ******************************************************************************/

void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location)
{
  snet_msg_data_op_t msg;
  MPI_Aint true_lb;
  MPI_Aint true_extent;
  MPI_Status status;
  MPI_Datatype type;
  int result = 0;
  void *buf = NULL;
  void *data = NULL;
  int count = 0;
  void *opt = NULL;

#ifdef SNET_DEBUG_COUNTERS 
  snet_time_t time_in;
  snet_time_t time_out;
  long mseconds;
  long payload;
#endif /* SNET_DEBUG_COUNTERS */

#ifdef SNET_DEBUG_COUNTERS 
  SNetDebugTimeGetTime(&time_in);
#endif /* SNET_DEBUG_COUNTERS */

  /* Acquire operation id: */
  pthread_mutex_lock(&storage.id_mutex);
    
  msg.op_id = storage.op_id++;
  
  if(msg.op_id == TAG_DATA_OP) {
    msg.op_id = TAG_DATA_OP + 1;
  }
  
  pthread_mutex_unlock(&storage.id_mutex);
    
  /* Start remote fetch: */
  msg.op = OP_fetch;
  msg.id = id;

  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);
 

  /* Receive and construct data type: */

  result = MPI_Probe(location, msg.op_id, storage.comm, &status);
  
  result = MPI_Get_count(&status, MPI_PACKED, &count);
  
  result = MPI_Type_get_true_extent(MPI_PACKED, &true_lb, &true_extent);
  
  buf = SNetMemAlloc(true_extent * count);  
  
  result = MPI_Recv(buf, count, MPI_PACKED, location, 
		    msg.op_id, storage.comm, &status);
  
  count = SNetGetDeserializeTypeFun(interface)(storage.comm, buf, count, &type, &opt);
  
  SNetMemFree(buf);


  /* Receive and construct the actual data: */

  result = MPI_Probe(location, msg.op_id, storage.comm, &status);
  
  result = MPI_Type_get_true_extent(type, &true_lb, &true_extent);
  
  
  buf = SNetMemAlloc(true_extent * count);

#ifdef SNET_DEBUG_COUNTERS 
  payload = true_extent * count;
#endif /* SNET_DEBUG_COUNTERS */


  result = MPI_Recv(buf, count, type, location, 
		    msg.op_id, storage.comm, &status);

  data = SNetGetUnpackFun(interface)(storage.comm, buf, type, count, opt);

#ifdef SNET_DEBUG_COUNTERS
  SNetDebugTimeGetTime(&time_out);

  mseconds = SNetDebugTimeDifferenceInMilliseconds(&time_in, &time_out);

  SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_DATA_TRANSFERS);

  SNetDebugCountersIncreaseCounter(payload, SNET_COUNTER_PAYLOAD_DATA_FETCHES);

  SNetDebugCountersIncreaseCounter(1, SNET_COUNTER_NUM_DATA_FETCHES);
  SNetDebugCountersIncreaseCounter(1, SNET_COUNTER_NUM_DATA_OPERATIONS);
#endif /* SNET_DEBUG_COUNTERS */

  return data;
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetDataStorageRemoteCopy(snet_id_t id, unsigned int location)
 *
 *   @brief  Copies a data on remote node.
 *
 *           Blocks until the data is copied.
 *
 *   @note   Causes a remote operation. 
 *   @note   Safe to use inside a locked reference!
 *
 *   @param id         The ID of the data to copy.
 *   @param location   Current location of the data.
 *
 ******************************************************************************/

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

  /* Reply is needed as the could otherwise be removed or fetched before its copied! */

  MPI_Recv(&msg, 0, MPI_INT, location, msg.op_id, storage.comm, &status);

#ifdef SNET_DEBUG_COUNTERS
  SNetDebugCountersIncreaseCounter(1, SNET_COUNTER_NUM_DATA_OPERATIONS);
#endif /* SNET_DEBUG_COUNTERS */
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location) 
 *
 *   @brief  Deletes a data on remote node.
 *
 *           Does not blocks until the data is deleted!
 *
 *   @note   Causes a remote operation. 
 *   @note   Safe to use inside a locked reference!
 *
 *   @param id         The ID of the data to delete.
 *   @param location   Current location of the data.
 *
 ******************************************************************************/

void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location) 
{
  snet_msg_data_op_t msg;

  msg.op = OP_delete;
  msg.op_id = 0;
  msg.id = id;

  pthread_mutex_lock(&storage.id_mutex);
    
  pthread_mutex_unlock(&storage.id_mutex);
  
  MPI_Send(&msg, 1, storage.op_type, location, TAG_DATA_OP, storage.comm);

#ifdef SNET_DEBUG_COUNTERS
  SNetDebugCountersIncreaseCounter(1, SNET_COUNTER_NUM_DATA_OPERATIONS);
#endif /* SNET_DEBUG_COUNTERS */
}

/*@}*/
#endif /* DISTRIBUTED_SNET */
