#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file reference.c
 *
 * @brief Implements reference system used in distributed S-Net.
 *
 * The reference system counts references to a data. 
 * 
 * Each node always contains a single reference for each (local or remote) data element that can be 
 * referred from the node and for each data element located in the node that can be remotely 
 * referred from other nodes.
 *
 * References are stored in data storage (@ref datastorage.h)
 *
 * The main functions of references is to hide remote data items by creating a local version of the 
 * reference and to make it possible to avoid remote data operations.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/

/* NOTICE:
 *
 * To avoid deadlocks between reference's and datastorage's mutexes, 
 * a thread holding mutex to a reference must release the mutex before 
 * calling any of the data storage's functions execept remote fetch, copy
 * and delte functions, which do not lock the data storage.
 *
 */

#include <pthread.h>

#include "reference.h"
#include "memfun.h"
#include "interface_functions.h"
#include "datastorage.h"

/** <!--********************************************************************-->
 *
 * @name Ref
 *
 * <!--
 * snet_ref_t *SNetRefCreate(snet_id_t id, int location, int interface, void *data) : Creates a new reference
 * snet_ref_t *SNetRefDestroy(snet_ref_t *ref) : Deletes the reference
 * snet_ref_t *SNetRefCopy(snet_ref_t *ref) : Copies the reference
 * int SNetRefGetRefCount(snet_ref_t *ref) : Returns the reference count
 * void *SNetRefGetData(snet_ref_t *ref) : Returns the data behind the reference
 * void *SNetRefTakeData(snet_ref_t *ref) : Returns the data behind the reference
 * int SNetRefGetInterface(snet_ref_t *ref) : Returns language interface
 * int SNetRefPack(snet_ref_t *ref, MPI_Comm comm, int *pos, void *buf, int buf_size) : Packs reference into buffer
 * snet_ref_t *SNetRefUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size) : Unpacks reference from buffer
 * -->
 *
 *****************************************************************************/
/*@{*/

/** @struct snet_ref
 *
 *   @brief Reference type.
 *
 */

struct snet_ref {
  snet_id_t id;           /**< Id of the data. */
  unsigned int location;  /**< Location of the data which is referred. */
  int interface;          /**< Language interface of the data. */
  unsigned int ref_count; /**< Count of references to the data.  */
  void *data;             /**< The actual data, or NULL if the data is not local. */
  pthread_mutex_t mutex;  /**< Mutex guarding access to the reference. */
  bool is_fetching;       /**< Information on if some thread is currently doing remote fetch. */
  pthread_cond_t ready;   /**< Condition used to signal ending of remote fetch. */
};


/** <!--********************************************************************-->
 *
 * @fn  static void SNetRefFetch(snet_ref_t *ref) 
 *
 *   @brief  Fetches the data from remote node, or waits until the data is
 *           fetched in case another thread is currently fetching the data.
 *
 *   @note   The call starts a remote fetch operation.   
 *
 *   @param ref  The reference.
 *
 ******************************************************************************/

static void SNetRefFetch(snet_ref_t *ref) 
{
  void *data = NULL;
  snet_id_t id;
  int interface;
  int location;

  if(!ref->is_fetching) {
    
    /* Fetch the data. */
    
    id = ref->id;
    interface = ref->interface;
    location = ref->location;
    
    ref->location = SNetIDServiceGetNodeID();
    
    ref->is_fetching = true;
    
    pthread_mutex_unlock(&ref->mutex);
    
    
    data = SNetDataStorageRemoteFetch(id, interface, location);
    
    
    pthread_mutex_lock(&ref->mutex);
    
    ref->data = data;
    
    ref->is_fetching = false;
    
    pthread_cond_signal(&ref->ready);
  } else {
    
    /* Wait until the data is fetched. */
    
    while(ref->is_fetching) {
      pthread_cond_wait(&ref->ready, &ref->mutex);
    }
  }

  return;
}


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetRefCreate(snet_id_t id, int location, int interface, void *data)
 *
 *   @brief  Creates a new reference, or copies an old reference with the same id.
 *
 *           If a reference is created for a remote data element, but there already
 *           is a reference with the same id, the reference is instead copied and
 *           the remote reference is deleted.
 *
 *   @note   In case of non-local data, the call may start a remote delete operation. 
 *
 *   @param id         Unique identifier of the data.
 *   @param location   Location of the data.
 *   @param interface  Langauge interface of the data. 
 *   @param data       The actual data, or NULL if the data is remote. 
 *
 *   @return           A new reference, or copy of an old reference.
 *
 ******************************************************************************/

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

    ref->is_fetching = false;
    
    pthread_cond_init(&ref->ready, NULL);

    ref->data = data;
    
    pthread_mutex_init(&ref->mutex, NULL);
    
    ref = SNetDataStoragePut(id, ref);

  } else if(location != SNetIDServiceGetNodeID()) {
    SNetDataStorageRemoteDelete(id, location);
  }

  return ref;
}


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetRefDestroy(snet_ref_t *ref) 
 *
 *   @brief  Destroys the given reference.
 *
 *           Because of the reference counting system, the reference count is 
 *           actually decreased by one, and the reference is only destroyed if 
 *           the reference count hits zero. In this case the reference is also 
 *           removed from the data storage.    
 *
 *   @note   In case of non-local data, the call may start a remote delete operation.   
 *
 *   @param ref  The reference to be destroyed.
 *
 *   @return     Pointer to the reference, or NULL if the reference was freed.
 *
 ******************************************************************************/


snet_ref_t *SNetRefDestroy(snet_ref_t *ref) 
{
  bool is_removed;

  pthread_mutex_lock(&ref->mutex);

  if(ref->ref_count == 1) {

    /* Try to remove the reference.
     *
     * It is possible that the reference is copied 
     * before it is removed from the data storage.
     * In this case the reference is not destroyed.
     *
     */

    do {
      pthread_mutex_unlock(&ref->mutex);
      
      is_removed = SNetDataStorageRemove(ref->id);
    
      pthread_mutex_lock(&ref->mutex);

     } while(!is_removed && ref->ref_count == 1);
      
    if(is_removed) {
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


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetRefCopy(snet_ref_t *ref)
 *
 *   @brief  Copies the given reference.
 *
 *           Increases the reference count of the data by one.      
 *
 *   @param ref  The reference to be copied.
 *
 *   @return     Pointer to the reference.
 *
 ******************************************************************************/

snet_ref_t *SNetRefCopy(snet_ref_t *ref)
{
  pthread_mutex_lock(&ref->mutex);

  ref->ref_count++;

  pthread_mutex_unlock(&ref->mutex);

  return ref;
}


/** <!--********************************************************************-->
 *
 * @fn  int SNetRefGetRefCount(snet_ref_t *ref)
 *
 *   @brief  Returns the reference count of the data the reference is referring to. 
 *
 *   @param ref  The reference.
 *
 *   @return     Count of references.
 *
 ******************************************************************************/

int SNetRefGetRefCount(snet_ref_t *ref)
{
  int ret;

  pthread_mutex_lock(&ref->mutex);

  ret = ref->ref_count;

  pthread_mutex_unlock(&ref->mutex);

  return ret;
}


/** <!--********************************************************************-->
 *
 * @fn  void *SNetRefGetData(snet_ref_t *ref)
 *
 *   @brief  Returns the data element behind the reference.
 *
 *           The data is not copied and the reference stays as it is. 
 *
 *           This function corresponds to SNetRecGetField()
 *
 *   @note   If the data is consumed, use SNetRefTakeData() instead!
 *
 *   @note   In case of non-local data, the call may start a remote fetch operation.   
 *
 *   @param ref  The reference.
 *
 *   @return     A pointer to the data element.
 *
 ******************************************************************************/

void *SNetRefGetData(snet_ref_t *ref)
{
  void *data = NULL;

  pthread_mutex_lock(&ref->mutex);

  if(ref->data == NULL) {

    SNetRefFetch(ref);
 
  }

  data = ref->data;

  pthread_mutex_unlock(&ref->mutex);

  return data;
}


/** <!--********************************************************************-->
 *
 * @fn  void *SNetRefTakeData(snet_ref_t *ref) 
 *
 *   @brief  Returns the data element behind the reference and destroys the reference.
 *
 *           If there are multiple references to the same data, the data is copied 
 *           using the language interface functions and a copy is returned. The 
 *           reference is destroyed as with SNetRefDestroy().  
 *
 *           This function corresponds to SNetRecTakeField()
 *
 *   @note   In case of non-local data, the call may start a remote fetch operation.   
 *
 *   @param ref  The reference.
 *
 *   @return     A pointer to the data element.
 *
 ******************************************************************************/

void *SNetRefTakeData(snet_ref_t *ref) 
{
  void *data = NULL;
  bool is_removed;

  pthread_mutex_lock(&ref->mutex);

  if(ref->data == NULL) {

    SNetRefFetch(ref);
   
  }

  if(ref->ref_count == 1) {

    /* Try to remove the reference.
     *
     * It is possible that the reference is copied 
     * before it is removed from the data storage.
     * In this case the reference is not destroyed.
     *
     */

    do {
      pthread_mutex_unlock(&ref->mutex);
      
      is_removed = SNetDataStorageRemove(ref->id);
      
      pthread_mutex_lock(&ref->mutex);

    } while(!is_removed && ref->ref_count == 1);

    if(is_removed) {

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


/** <!--********************************************************************-->
 *
 * @fn  int SNetRefGetInterface(snet_ref_t *ref)
 * 
 *   @brief  Returns the language interface of the data element.
 *
 *   @param ref  The reference.
 *
 *   @return     Langauge interface id of the data.
 *
 ******************************************************************************/

int SNetRefGetInterface(snet_ref_t *ref) 
{
  /* No synchronization is needed as this value cannot change! */

  return ref->interface;
}


/** <!--********************************************************************-->
 *
 * @fn  int SNetRefPack(snet_ref_t *ref, MPI_Comm comm, int *pos, void *buf, int buf_size)
 *
 *   @brief  Packs presentation of the reference into a buffer with MPI_Pack.
 *
 *           In case of non-local data, the reference is moved to point to
 *           the real reference (in the node where the data resides).           
 *
 *   @note   In case of non-local data, the call may start a remote copy operation.   
 *
 *   @param ref       The reference.
 *   @param comm      The MPI communicator used to send the buffered data  (as in MPI_Pack).
 *   @param pos       Position in the buffer (as in MPI_Pack).
 *   @param buf       The actual buffer into which the reference is packed.
 *   @param buf_size  The size of the buffer.
 *
 *   @return          MPI_SUCCESS in case of success or result of MPI_Pack otherwise.
 *
 ******************************************************************************/

int SNetRefPack(snet_ref_t *ref, MPI_Comm comm, int *pos, void *buf, int buf_size)
{
  int result;
  bool is_removed;

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
  
  if(ref->data == NULL && !ref->is_fetching) {
    if(ref->ref_count == 1) {

      /* Try to remove the reference.
       *
       * It is possible that the reference is copied 
       * before it is removed from the data storage.
       * In this case the reference is not destroyed.
       *
       */

      do {
	pthread_mutex_unlock(&ref->mutex);

	is_removed = SNetDataStorageRemove(ref->id);

	pthread_mutex_lock(&ref->mutex);

      } while(!is_removed && ref->ref_count == 1);


      if(is_removed) {

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


/** <!--********************************************************************-->
 *
 * @fn  snet_ref_t *SNetRefUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size)
 *
 *   @brief  Constructs a reference from a presentation packed with SNetRefPack
 *
 *           In case of non-local data, the reference is moved to point to
 *           the real reference (in the node where the data resides).           
 *
 *   @note   In case of non-local data, the call may start a remote delete operation.   
 *
 *   @param comm      The MPI communicator used to send the buffered data  (as in MPI_Unpack).
 *   @param pos       Position in the buffer (as in MPI_Unpack).
 *   @param buf       The actual buffer from which the reference is unpacked.
 *   @param buf_size  The size of the buffer.
 *
 *   @return          A pointer to the reference in case of success, or NULL otherwise.
 *
 ******************************************************************************/

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
/*@}*/

#endif /* DISTRIBUTED_SNET */
