#include <assert.h>
#include <pthread.h>

#include "distribution.h"
#include "entities.h"
#include "interface_functions.h"
#include "iomanagers.h"
#include "memfun.h"
#include "refcollections.h"
#include "stream.h"

static snet_ref_data_map_t *dataMap = NULL;
static pthread_mutex_t dsMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetRefStart(void)
{
  pthread_mutex_lock(&dsMutex);
  dataMap = SNetRefDataMapCreate(0);
  pthread_mutex_unlock(&dsMutex);
}

snet_ref_t *SNetRefCreate(void *data, int interface)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));

  result->node = SNetDistribGetNodeId();
  result->interface = interface;
  result->data = (uintptr_t) data;

  return result;
}

snet_ref_t *SNetRefCopy(snet_ref_t *ref)
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
      *result = *ref;
      tmp->refs++;
      SNetDistribRemoteUpdate(ref, 1);
    } else {
      result->node = SNetDistribGetNodeId();
      result->interface = ref->interface;
      result->data = (uintptr_t) copyfun(tmp->data);
    }
    pthread_mutex_unlock(&dsMutex);
  }

  return result;
}

static void *GetData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) {
    result = (void*) ref->data;
  } else {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    tmp->refs--;

    if (tmp->data == NULL) {
      snet_stream_t *str = SNetStreamCreate(1);
      snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

      if (tmp->list == NULL) {
        tmp->list = SNetStreamListCreate(1, str);
        SNetDistribRemoteFetch(ref);
      } else {
        SNetStreamListAppendEnd(tmp->list, str);
      }
      pthread_mutex_unlock(&dsMutex);
      SNetStreamRead(sd);
      SNetStreamClose(sd, true);
      pthread_mutex_lock(&dsMutex);
    } else SNetDistribRemoteUpdate(ref, -1);

    if (tmp->refs == 0) {
      result = tmp->data;
      SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
    } else {
      result = SNetInterfaceGet(ref->interface)->copyfun(tmp->data);
    }

    pthread_mutex_unlock(&dsMutex);
  }
  ref->node = SNetDistribGetNodeId();
  ref->data = (uintptr_t) result;

  return result;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  return GetData(ref);
}

void *SNetRefTakeData(snet_ref_t *ref)
{
  void *result = GetData(ref);
  SNetMemFree(ref);
  return result;
}

void SNetRefDestroy(snet_ref_t *ref)
{
  if (ref->node == 0 && ref->interface == 0 && ref->data == 0) return;

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
      SNetDistribRemoteUpdate(ref, -1);
    }
    pthread_mutex_unlock(&dsMutex);
  }

  SNetMemFree(ref);
}

snet_ref_t *SNetRefIncoming(int node, int interface, uintptr_t data)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));

  result->node = node;
  result->interface = interface;
  result->data = data;

  if (SNetDistribIsNodeLocation(node)) {
    pthread_mutex_lock(&dsMutex);
    data_t *tmp = SNetRefDataMapGet(dataMap, *result);
    tmp->refs--;

    if (tmp->refs == 0) {
      result->data = (uintptr_t) tmp->data;
      SNetMemFree(SNetRefDataMapTake(dataMap, *result));
    } else {
      result->data = (uintptr_t) SNetInterfaceGet(interface)->copyfun(tmp->data);
    }

    pthread_mutex_unlock(&dsMutex);
  } else {
    data_t *tmp;
    pthread_mutex_lock(&dsMutex);
    if (SNetRefDataMapContains(dataMap, *result)) {
      tmp = SNetRefDataMapGet(dataMap, *result);
      tmp->refs++;
    } else {
      tmp = SNetMemAlloc(sizeof(data_t));
      tmp->refs = 1;
      tmp->data = NULL;
      tmp->list = NULL;
      SNetRefDataMapSet(dataMap, *result, tmp);
    }
    pthread_mutex_unlock(&dsMutex);
  }

  return result;
}

void SNetRefOutgoing(snet_ref_t *ref)
{
  pthread_mutex_lock(&dsMutex);
  if (SNetRefDataMapContains(dataMap, *ref)) {
    data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
    if (SNetDistribIsNodeLocation(ref->node)) tmp->refs++;
    else tmp->refs--;

    if (tmp->refs == 0) SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  } else {
    data_t *tmp = SNetMemAlloc(sizeof(data_t));
    snet_copy_fun_t copyfun= SNetInterfaceGet(ref->interface)->copyfun;
    tmp->refs = 1;
    tmp->data = copyfun((void*) ref->data);
    tmp->list = NULL;
    SNetRefDataMapSet(dataMap, *ref, tmp);
  }
  pthread_mutex_unlock(&dsMutex);
  ref->node = 0;
  ref->interface = 0;
  ref->data = 0;
}

void *SNetRefFetch(snet_ref_t *ref)
{
  void *result;
  snet_copy_fun_t copyfun= SNetInterfaceGet(ref->interface)->copyfun;
  pthread_mutex_lock(&dsMutex);
  assert(SNetDistribIsNodeLocation(ref->node));
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  tmp->refs--;
  if (tmp->refs == 0) {
    result = tmp->data;
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  } else {
    result = copyfun(tmp->data);
  }
  pthread_mutex_unlock(&dsMutex);
  return result;
}

void SNetRefUpdate(snet_ref_t *ref, int value)
{
  snet_free_fun_t freefun = SNetInterfaceGet(ref->interface)->freefun;
  pthread_mutex_lock(&dsMutex);
  assert(SNetDistribIsNodeLocation(ref->node));
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  tmp->refs = tmp->refs + value;
  if (tmp->refs == 0) {
    freefun(tmp->data);
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  }
  pthread_mutex_unlock(&dsMutex);
}

void SNetRefSet(snet_ref_t *ref, void *data)
{
  snet_stream_t *str;
  snet_stream_desc_t *sd;
  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  tmp->data = data;
  LIST_FOR_EACH(tmp->list, str)
    sd = SNetStreamOpen(str, 'w');
    SNetStreamWrite(sd, (void*) 1);
    SNetStreamClose(sd, false);
  END_FOR

  SNetStreamListDestroy(tmp->list);
  tmp->list = NULL;
  pthread_mutex_unlock(&dsMutex);
}
