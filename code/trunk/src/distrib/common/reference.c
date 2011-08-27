#include <assert.h>
#include <pthread.h>

#include "distribution.h"
#include "entities.h"
#include "interface_functions.h"
#include "iomanagers.h"
#include "memfun.h"
#include "refmap.h"

static snet_ref_data_map_t *dataMap = NULL;
static pthread_mutex_t dsMutex = PTHREAD_MUTEX_INITIALIZER;

snet_ref_t *SNetDistribRefCreate(void *data, int interface)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));

  result->node = SNetDistribGetNodeId();
  result->interface = interface;
  result->data = (uintptr_t) data;

  return result;
}

snet_ref_t *SNetDistribRefCopy(snet_ref_t *ref)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));
  snet_copy_fun_t copyfun= SNetInterfaceGet(ref->interface)->copyfun;

  if (SNetDistribIsNodeLocation(ref->node)) {
    result->node = ref->node;
    result->interface = ref->interface;
    result->data = (uintptr_t) copyfun((void*) ref->data);
  } else {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    if (tmp->data == NULL) {
      tmp->refs++;
      //Message remote node with copy
    } else {
      result->node = SNetDistribGetNodeId();
      result->interface = ref->interface;
      result->data = (uintptr_t) copyfun(tmp->data);
    }
    pthread_mutex_unlock(&dsMutex);
  }

  return result;
}

void *SNetDistribRefGetData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) {
    result = (void*) ref->data;
  } else {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    tmp->refs--;
    if (tmp->data == NULL) {
      //Remote fetch operation (also decrements by one)
    } else {
      //Remote delete operation
    }

    if (tmp->refs == 0) {
      result = tmp->data;
      SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
      ref->data = (uintptr_t) result;
    } else {
      result = SNetInterfaceGet(ref->interface)->copyfun(tmp->data);
      ref->data = (uintptr_t) result;
    }

    ref->node = SNetDistribGetNodeId();

    pthread_mutex_unlock(&dsMutex);
  }

  return result;
}

void *SNetDistribRefTakeData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) {
    result = (void*) ref->data;
  } else {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    tmp->refs--;
    if (tmp->data == NULL) {
      //Remote fetch operation (also decrements by one)
    } else {
      //Remote delete operation
    }

    if (tmp->refs == 0) {
      result = tmp->data;
      SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
    } else {
      result = SNetInterfaceGet(ref->interface)->copyfun(tmp->data);
      ref->data = (uintptr_t) result;
    }

    pthread_mutex_unlock(&dsMutex);
  }

  SNetMemFree(ref);

  return result;
}

void SNetDistribRefDestroy(snet_ref_t *ref)
{
  snet_free_fun_t freefun = SNetInterfaceGet(ref->interface)->freefun;

  if (SNetDistribIsNodeLocation(ref->node)) {
    freefun((void*) ref->data);
  } else {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    if (tmp->data != NULL) {
      tmp->refs--;
      if (tmp->refs == 0) {
        freefun(tmp->data);
        SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
      }
    } else {
      //Remote destroy
    }
    pthread_mutex_unlock(&dsMutex);
  }

  SNetMemFree(ref);
}
