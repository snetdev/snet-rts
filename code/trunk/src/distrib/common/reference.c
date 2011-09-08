#include <assert.h>
#include <pthread.h>

#include "distribution.h"
#include "entities.h"
#include "interface_functions.h"
#include "iomanagers.h"
#include "memfun.h"
#include "refcollections.h"
#include "reference.h"
#include "stream.h"
#include "threading.h"

#define COPYFUN(interface, data)    (SNetInterfaceGet(interface)->copyfun(data))
#define FREEFUN(interface, data)    (SNetInterfaceGet(interface)->freefun(data))

struct snet_refcount {
  int count;
  void *data;
  snet_stream_list_t *list;
};

static snet_ref_refcount_map_t *refcountMap = NULL;
static pthread_mutex_t refMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetReferenceInit(void)
{
  pthread_mutex_lock(&refMutex);
  refcountMap = SNetRefRefcountMapCreate(0);
  pthread_mutex_unlock(&refMutex);
}

void SNetReferenceDestroy(void)
{
  pthread_mutex_destroy(&refMutex);
  assert(SNetRefRefcountMapSize(refcountMap) == 0);
  SNetRefRefcountMapDestroy(refcountMap);
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

  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  if (tmp->data) {
    result->node = SNetDistribGetNodeId();
    result->data = (uintptr_t) COPYFUN(ref->interface, tmp->data);
  } else {
    tmp->count++;
    SNetDistribRemoteRefUpdate(ref, 1);
  }
  pthread_mutex_unlock(&refMutex);

  return result;
}

static void *getData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) return (void*) ref->data;

  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);

  if (tmp->data) {
    SNetDistribRemoteRefUpdate(ref, -1);
  } else {
    snet_stream_t *str = SNetStreamCreate(1);
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

    if (tmp->list) {
      SNetStreamListAppendEnd(tmp->list, str);
    } else {
      tmp->list = SNetStreamListCreate(1, str);
      SNetDistribRemoteRefFetch(ref);
    }

    pthread_mutex_unlock(&refMutex);
    SNetStreamRead(sd);
    SNetStreamClose(sd, true);
    pthread_mutex_lock(&refMutex);
  }

  if (--tmp->count == 0) {
    result = tmp->data;
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  } else {
    result = COPYFUN(ref->interface, tmp->data);
  }

  pthread_mutex_unlock(&refMutex);

  return result;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  ref->node = SNetDistribGetNodeId();
  ref->data = (uintptr_t) getData(ref);
  return (void*) ref->data;
}

void *SNetRefTakeData(snet_ref_t *ref)
{
  void *result = getData(ref);
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

  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);

  SNetDistribRemoteRefUpdate(ref, -1);
  if (--tmp->count == 0) {
    if (tmp->data) FREEFUN(ref->interface, tmp->data);
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  }
  pthread_mutex_unlock(&refMutex);

  SNetMemFree(ref);
}

snet_ref_t *SNetRefIncoming(snet_ref_t *ref)
{
  snet_refcount_t *tmp;
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));
  *result = *ref;

  pthread_mutex_lock(&refMutex);
  if (SNetRefRefcountMapContains(refcountMap, *ref)) {
    tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  } else {
    tmp = SNetMemAlloc(sizeof(snet_refcount_t));
    tmp->count = 1;
    tmp->data = tmp->list = NULL;
    SNetRefRefcountMapSet(refcountMap, *result, tmp);
  }

  if (!SNetDistribIsNodeLocation(ref->node)) {
    tmp->count++;
  } else if (--tmp->count == 0) {
    result->data = (uintptr_t) tmp->data;
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  } else {
    result->data = (uintptr_t) COPYFUN(ref->interface, tmp->data);
  }
  pthread_mutex_unlock(&refMutex);

  return result;
}

void SNetRefOutgoing(snet_ref_t *ref)
{
  snet_refcount_t *tmp;
  pthread_mutex_lock(&refMutex);
  if (SNetRefRefcountMapContains(refcountMap, *ref)) {
    tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  } else {
    tmp = SNetMemAlloc(sizeof(snet_refcount_t));
    tmp->count = 0;
    tmp->data = (void*) ref->data;
    tmp->list = NULL;
    SNetRefRefcountMapSet(refcountMap, *ref, tmp);
  }

  if (SNetDistribIsNodeLocation(ref->node)) {
    tmp->count++;
  } else if (--tmp->count == 0) {
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  }
  pthread_mutex_unlock(&refMutex);
  ref->node = ref->interface = ref->data = 0;
}

void *SNetRefFetch(snet_ref_t *ref)
{
  void *result;
  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  if (--tmp->count == 0) {
    result = tmp->data;
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  } else {
    result = COPYFUN(ref->interface, tmp->data);
  }
  pthread_mutex_unlock(&refMutex);
  return result;
}

void SNetRefUpdate(snet_ref_t *ref, int value)
{
  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  tmp->count = tmp->count + value;
  if (tmp->count == 0) {
    FREEFUN(ref->interface, tmp->data);
    SNetMemFree(SNetRefRefcountMapTake(refcountMap, *ref));
  }
  pthread_mutex_unlock(&refMutex);
}

void SNetRefSet(snet_ref_t *ref, void *data)
{
  snet_stream_t *str;
  pthread_mutex_lock(&refMutex);
  snet_refcount_t *tmp = SNetRefRefcountMapGet(refcountMap, *ref);
  tmp->data = data;

  LIST_DEQUEUE_EACH(tmp->list, str) {
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'w');
    SNetStreamWrite(sd, (void*) 1);
    SNetStreamClose(sd, false);
  }

  SNetStreamListDestroy(tmp->list);
  pthread_mutex_unlock(&refMutex);
}
