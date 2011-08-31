#include <assert.h>
#include <pthread.h>

#include "distribution.h"
#include "entities.h"
#include "interface_functions.h"
#include "iomanagers.h"
#include "memfun.h"
#include "refcollections.h"
#include "stream.h"

#define COPYFUN(interface, data)    (SNetInterfaceGet(interface)->copyfun(data))
#define FREEFUN(interface, data)    (SNetInterfaceGet(interface)->freefun(data))

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
  *result = *ref;

  if (SNetDistribIsNodeLocation(ref->node)) {
    result->data = (uintptr_t) COPYFUN(ref->interface, (void*) ref->data);
    return result;
  }

  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  if (tmp->data) {
    result->node = SNetDistribGetNodeId();
    result->data = (uintptr_t) COPYFUN(ref->interface, tmp->data);
  } else {
    tmp->refs++;
    SNetDistribRemoteUpdate(ref, 1);
  }
  pthread_mutex_unlock(&dsMutex);

  return result;
}

static void *GetData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) return (void*) ref->data;

  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);

  if (tmp->data) {
    SNetDistribRemoteUpdate(ref, -1);
  } else {
    snet_stream_t *str = SNetStreamCreate(1);
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

    if (tmp->list) {
      SNetStreamListAppendEnd(tmp->list, str);
    } else {
      tmp->list = SNetStreamListCreate(1, str);
      SNetDistribRemoteFetch(ref);
    }

    pthread_mutex_unlock(&dsMutex);
    SNetStreamRead(sd);
    SNetStreamClose(sd, true);
    pthread_mutex_lock(&dsMutex);
  }

  if (--tmp->refs == 0) {
    result = tmp->data;
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  } else {
    result = COPYFUN(ref->interface, tmp->data);
  }

  pthread_mutex_unlock(&dsMutex);

  return result;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  ref->node = SNetDistribGetNodeId();
  ref->data = (uintptr_t) GetData(ref);
  return (void*) ref->data;
}

void *SNetRefTakeData(snet_ref_t *ref)
{
  void *result = GetData(ref);
  SNetMemFree(ref);
  return result;
}

void SNetRefDestroy(snet_ref_t *ref)
{
  if (ref->node == 0 && ref->interface == 0 && ref->data == 0) {
    SNetMemFree(ref);
    return;
  }

  if (SNetDistribIsNodeLocation(ref->node)) {
    FREEFUN(ref->interface, (void*) ref->data);
    SNetMemFree(ref);
    return;
  }

  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  if (tmp->data) SNetDistribRemoteUpdate(ref, -1);

  tmp->refs--;
  if (tmp->refs == 0) {
    if (tmp->data) FREEFUN(ref->interface, tmp->data);
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  }
  pthread_mutex_unlock(&dsMutex);

  SNetMemFree(ref);
}

snet_ref_t *SNetRefIncoming(snet_ref_t ref)
{
  data_t *tmp;
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));
  *result = ref;

  pthread_mutex_lock(&dsMutex);
  if (SNetRefDataMapContains(dataMap, ref)) {
    tmp = SNetRefDataMapGet(dataMap, ref);
  } else {
    tmp = SNetMemAlloc(sizeof(data_t));
    tmp->refs = 1;
    tmp->data = tmp->list = NULL;
    SNetRefDataMapSet(dataMap, *result, tmp);
  }

  if (!SNetDistribIsNodeLocation(ref.node)) {
    tmp->refs++;
  } else if (--tmp->refs == 0) {
    result->data = (uintptr_t) tmp->data;
    SNetMemFree(SNetRefDataMapTake(dataMap, ref));
  } else {
    result->data = (uintptr_t) COPYFUN(ref.interface, tmp->data);
  }
  pthread_mutex_unlock(&dsMutex);

  return result;
}

void SNetRefOutgoing(snet_ref_t *ref)
{
  data_t *tmp;
  pthread_mutex_lock(&dsMutex);
  if (SNetRefDataMapContains(dataMap, *ref)) {
    tmp = SNetRefDataMapGet(dataMap, *ref);
  } else {
    tmp = SNetMemAlloc(sizeof(data_t));
    tmp->refs = 0;
    tmp->data = (void*) ref->data;
    tmp->list = NULL;
    SNetRefDataMapSet(dataMap, *ref, tmp);
  }

  if (SNetDistribIsNodeLocation(ref->node)) {
    tmp->refs++;
  } else if (--tmp->refs == 0) {
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  }
  pthread_mutex_unlock(&dsMutex);
  ref->node = ref->interface = ref->data = 0;
}

void *SNetRefFetch(snet_ref_t *ref)
{
  void *result;
  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  if (--tmp->refs == 0) {
    result = tmp->data;
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  } else {
    result = COPYFUN(ref->interface, tmp->data);
  }
  pthread_mutex_unlock(&dsMutex);
  return result;
}

void SNetRefUpdate(snet_ref_t *ref, int value)
{
  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  tmp->refs = tmp->refs + value;
  if (tmp->refs == 0) {
    FREEFUN(ref->interface, tmp->data);
    SNetMemFree(SNetRefDataMapTake(dataMap, *ref));
  }
  pthread_mutex_unlock(&dsMutex);
}

void SNetRefSet(snet_ref_t *ref, void *data)
{
  snet_stream_t *str;
  pthread_mutex_lock(&dsMutex);
  data_t *tmp = SNetRefDataMapGet(dataMap, *ref);
  tmp->data = data;

  LIST_FOR_EACH(tmp->list, str)
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'w');
    SNetStreamWrite(sd, (void*) 1);
    SNetStreamClose(sd, false);
  END_FOR

  SNetStreamListDestroy(tmp->list);
  tmp->list = NULL;
  pthread_mutex_unlock(&dsMutex);
}
