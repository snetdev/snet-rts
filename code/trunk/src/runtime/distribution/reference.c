#ifdef DISTRIBUTED_SNET

#include <pthread.h>

#include "reference.h"
#include "memfun.h"
#include "interface_functions.h"
#include "datastorage.h"

struct snet_ref {
  snet_id_t id;
  unsigned int location;
  int interface;
  unsigned int ref_count;
  void *data;
  pthread_mutex_t mutex; 
};

snet_ref_t *SNetRefCreate(snet_id_t id, int location, 
			  int interface, void *data)
{
  snet_ref_t *ref;

  if(location != SNetIDServiceGetNodeID()) {
    ref =  SNetDataStorageCopy(id);
  } else {
    ref = SNetDataStorageGet(id);
  }
  
  if(ref == NULL) {
    ref = SNetMemAlloc(sizeof(snet_ref_t));
    
    ref->id = id;
    ref->location = location;
    ref->interface = interface;
    ref->ref_count = 1;

    ref->data = data;
    
    pthread_mutex_init(&ref->mutex, NULL);
    
    ref = SNetDataStoragePut(id, ref);

  } else if(location != SNetIDServiceGetNodeID()) {
    SNetDataStorageRemoteDelete(id, location);
  }

  return ref;
}

snet_ref_t *SNetRefDestroy(snet_ref_t *ref) 
{
  bool b;

  pthread_mutex_lock(&ref->mutex);

  if(ref->ref_count == 1) {

    do {
      pthread_mutex_unlock(&ref->mutex);
      
      b = SNetDataStorageRemove(ref->id);
    
      pthread_mutex_lock(&ref->mutex);

     } while(!b && ref->ref_count == 1);
      
    if(b) {
      if(ref->data != NULL) {
	SNetGetFreeFun(ref->interface)(ref->data);
      } else {
	SNetDataStorageRemoteDelete(ref->id, ref->location);
      }

      pthread_mutex_unlock(&ref->mutex);
      
      pthread_mutex_destroy(&ref->mutex);
      
      SNetMemFree(ref);
      
      ref = NULL;
      
    } else {
      ref->ref_count--;

      pthread_mutex_unlock(&ref->mutex);
    }
    
  } else {
    ref->ref_count--;

    pthread_mutex_unlock(&ref->mutex);
  }

  return ref;
}

snet_ref_t *SNetRefCopy(snet_ref_t *ref)
{
  pthread_mutex_lock(&ref->mutex);

  ref->ref_count++;

  pthread_mutex_unlock(&ref->mutex);

  return ref;
}

int SNetRefGetRefCount(snet_ref_t *ref)
{
  int ret;

  pthread_mutex_lock(&ref->mutex);

  ret = ref->ref_count;

  pthread_mutex_unlock(&ref->mutex);

  return ret;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  void *data = NULL;

  pthread_mutex_lock(&ref->mutex);

  if(ref->data == NULL) {
    ref->data = SNetDataStorageRemoteFetch(ref->id, ref->interface, ref->location);
    ref->location = SNetIDServiceGetNodeID();
  }

  data = ref->data;

  pthread_mutex_unlock(&ref->mutex);

  return data;
}

void *SNetRefTakeData(snet_ref_t *ref) 
{
  void *data;
  bool b;

  pthread_mutex_lock(&ref->mutex);

  if(ref->data == NULL) {
    ref->data = SNetDataStorageRemoteFetch(ref->id, ref->interface, ref->location);
    ref->location = SNetIDServiceGetNodeID();
  }

  if(ref->ref_count == 1) {
    do {
      pthread_mutex_unlock(&ref->mutex);
      
      b = SNetDataStorageRemove(ref->id);
      
      pthread_mutex_lock(&ref->mutex);

    } while(!b && ref->ref_count == 1);

    if(b) {

      data = ref->data;

      pthread_mutex_unlock(&ref->mutex);
    
      pthread_mutex_destroy(&ref->mutex);

      SNetMemFree(ref);
    } else {
      ref->ref_count--;

      data = ref->data;

      data = SNetGetCopyFun(ref->interface)(ref->data);

      pthread_mutex_unlock(&ref->mutex);
    }
  } else {
    ref->ref_count--;
 
    data = ref->data;

    data = SNetGetCopyFun(ref->interface)(ref->data);

    pthread_mutex_unlock(&ref->mutex);
  } 

  return data; 
}

int SNetRefGetInterface(snet_ref_t *ref) 
{
  /* No synchronization is needed as this value cannot change! */

  return ref->interface;
}

int SNetRefPack(snet_ref_t *ref, MPI_Comm comm, int *pos, void *buf, int buf_size)
{
  int result;
  bool b;

  pthread_mutex_lock(&ref->mutex);

  if((result = MPI_Pack(&ref->id, 1, SNET_ID_MPI_TYPE, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
    pthread_mutex_unlock(&ref->mutex);
    return result;
  }
      
  if((result = MPI_Pack(&ref->location, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
    pthread_mutex_unlock(&ref->mutex);
    return result;
  }

  if((result = MPI_Pack(&ref->interface, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
    pthread_mutex_unlock(&ref->mutex);
    return result;
  }
  
  if(ref->data == NULL) {
    if(ref->ref_count == 1) {
      do {
	pthread_mutex_unlock(&ref->mutex);

	b = SNetDataStorageRemove(ref->id);

	pthread_mutex_lock(&ref->mutex);

      } while(!b && ref->ref_count == 1);


      if(b) {

	pthread_mutex_unlock(&ref->mutex);

	pthread_mutex_destroy(&ref->mutex);

	SNetMemFree(ref);

	ref = NULL;

      } else {
	ref->ref_count--;

	SNetDataStorageRemoteCopy(ref->id, ref->location);

	pthread_mutex_unlock(&ref->mutex);

      }    
    } else {
      ref->ref_count--;

      SNetDataStorageRemoteCopy(ref->id, ref->location);
      
      pthread_mutex_unlock(&ref->mutex);
    }
    
  } else {
    pthread_mutex_unlock(&ref->mutex);
  }

  return MPI_SUCCESS;
}

snet_ref_t *SNetRefUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size)
{
  snet_id_t id;
  int location;
  int interface;

  if(MPI_Unpack(buf, buf_size, pos, &id, 1, SNET_ID_MPI_TYPE, comm) != MPI_SUCCESS) {
    return NULL;
  }

  if(MPI_Unpack(buf, buf_size, pos, &location, 1, MPI_INT, comm) != MPI_SUCCESS) {
    return NULL;
  }

  if(MPI_Unpack(buf, buf_size, pos, &interface, 1, MPI_INT, comm) != MPI_SUCCESS) {
    return NULL;
  }

  return SNetRefCreate(id, location, interface, NULL);
}
#endif /* DISTRIBUTED_SNET */
