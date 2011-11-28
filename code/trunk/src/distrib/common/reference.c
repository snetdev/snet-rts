#include <assert.h>
#include <pthread.h>

#include "distribcommon.h"
#include "distribution.h"
#include "entities.h"
#include "interface_functions.h"
#include "memfun.h"
#include "reference.h"
#include "stream.h"
#include "threading.h"

#define COPYFUN(interface, data)    (SNetInterfaceGet(interface)->copyfun((void*) (data)))
#define FREEFUN(interface, data)    (SNetInterfaceGet(interface)->freefun((void*) (data)))

#define LIST_NAME_H Stream
#define LIST_TYPE_NAME_H stream
#define LIST_VAL_H snet_stream_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define MAP_NAME_H RefRefcount
#define MAP_TYPE_NAME_H ref_refcount
#define MAP_KEY_H snet_ref_t
#define MAP_VAL_H snet_refcount_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

struct snet_ref {
    int node, interface;
    uintptr_t data;
};

struct snet_refcount {
  int count;
  void *data;
  snet_stream_list_t *list;
};

#define LIST_NAME Stream
#define LIST_TYPE_NAME stream
#define LIST_VAL snet_stream_t*
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

bool SNetRefCompare(snet_ref_t r1, snet_ref_t r2)
{
  return r1.node == r2.node && r1.data == r2.data;
}

#define MAP_NAME RefRefcount
#define MAP_TYPE_NAME ref_refcount
#define MAP_KEY snet_ref_t
#define MAP_VAL snet_refcount_t*
#define MAP_KEY_CMP SNetRefCompare
#include "map-template.c"
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

static snet_ref_refcount_map_t *localRefMap = NULL;
static snet_ref_refcount_map_t *remoteRefMap = NULL;
static pthread_mutex_t localRefMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t remoteRefMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetReferenceInit(void)
{
  localRefMap = SNetRefRefcountMapCreate(0);
  remoteRefMap = SNetRefRefcountMapCreate(0);
}

void SNetReferenceDestroy(void)
{
  pthread_mutex_destroy(&localRefMutex);
  pthread_mutex_destroy(&remoteRefMutex);
  assert(SNetRefRefcountMapSize(localRefMap) == 0);
  assert(SNetRefRefcountMapSize(remoteRefMap) == 0);
  SNetRefRefcountMapDestroy(localRefMap);
  SNetRefRefcountMapDestroy(remoteRefMap);
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
    result->data = (uintptr_t) COPYFUN(ref->interface, ref->data);
  } else {
    pthread_mutex_lock(&remoteRefMutex);
    snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

    if (refInfo->data) {
      result->node = SNetDistribGetNodeId();
      result->data = (uintptr_t) COPYFUN(result->interface, refInfo->data);
    } else {
      refInfo->count++;
      SNetDistribUpdateRef(ref, 1);
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }

  return result;
}

void SNetRefSerialise(snet_ref_t *ref, void *buf,
                      void (*serialiseInt)(void*, int, int*),
                      void (*serialiseByte)(void*, int, char*))
{
    serialiseInt(buf, 1, &ref->node);
    serialiseInt(buf, 1, &ref->interface);
    serialiseByte(buf, sizeof(uintptr_t), (char*) &ref->data);
}

void SNetRefDeserialise(snet_ref_t *ref, void *buf,
                        void (*deserialiseInt)(void*, int, int*),
                        void (*deserialiseByte)(void*, int, char*))
{
    deserialiseInt(buf, 1, &ref->node);
    deserialiseInt(buf, 1, &ref->interface);
    deserialiseByte(buf, sizeof(uintptr_t), (char*) &ref->data);
}

snet_ref_t *SNetRefAlloc(void)
{ return SNetMemAlloc(sizeof(snet_ref_t)); }

int SNetRefInterface(snet_ref_t *ref)
{ return ref->interface; };

int SNetRefNode(snet_ref_t *ref)
{ return ref->node; };

static void *GetData(snet_ref_t *ref)
{
  void *result;
  if (SNetDistribIsNodeLocation(ref->node)) return (void*) ref->data;

  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  if (refInfo->data == NULL) {
    snet_stream_t *str = SNetStreamCreate(1);
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

    if (refInfo->list) {
      SNetStreamListAppendEnd(refInfo->list, str);
    } else {
      refInfo->list = SNetStreamListCreate(1, str);
      SNetDistribFetchRef(ref);
    }

    // refInfo->count gets updated by SNetRefSet instead of here to avoid a
    // race between nodes when a pointer is recycled.
    pthread_mutex_unlock(&remoteRefMutex);
    result = SNetStreamRead(sd);
    SNetStreamClose(sd, true);

    return result;
  }

  SNetDistribUpdateRef(ref, -1);
  if (--refInfo->count == 0) {
    result = refInfo->data;
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  } else {
    result = COPYFUN(ref->interface, refInfo->data);
  }

  pthread_mutex_unlock(&remoteRefMutex);
  return result;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  ref->data = (uintptr_t) GetData(ref);
  ref->node = SNetDistribGetNodeId();
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
  if (SNetDistribIsNodeLocation(ref->node)) {
    FREEFUN(ref->interface, ref->data);
    SNetMemFree(ref);
    return;
  }

  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  SNetDistribUpdateRef(ref, -1);
  if (--refInfo->count == 0) {
    if (refInfo->data) FREEFUN(ref->interface, refInfo->data);
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  }
  pthread_mutex_unlock(&remoteRefMutex);

  SNetMemFree(ref);
}

void SNetRefIncoming(snet_ref_t *ref)
{
  snet_refcount_t *refInfo;

  if (SNetDistribIsNodeLocation(ref->node)) {
    pthread_mutex_lock(&localRefMutex);

    refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
    if (--refInfo->count == 0) {
      SNetMemFree(SNetRefRefcountMapTake(localRefMap, *ref));
    } else {
      ref->data = (uintptr_t) COPYFUN(ref->interface, ref->data);
    }

    pthread_mutex_unlock(&localRefMutex);
  } else {
    pthread_mutex_lock(&remoteRefMutex);

    if (SNetRefRefcountMapContains(remoteRefMap, *ref)) {
      refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);
      refInfo->count++;
    } else {
      refInfo = SNetMemAlloc(sizeof(snet_refcount_t));
      refInfo->count = 1;
      refInfo->data = refInfo->list = NULL;
      SNetRefRefcountMapSet(remoteRefMap, *ref, refInfo);
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }
}

void SNetRefOutgoing(snet_ref_t *ref)
{
  snet_refcount_t *refInfo;

  if (SNetDistribIsNodeLocation(ref->node)) {
    pthread_mutex_lock(&localRefMutex);

    if (SNetRefRefcountMapContains(localRefMap, *ref)) {
      refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
      refInfo->count++;
      FREEFUN(ref->interface, ref->data);
    } else {
      refInfo = SNetMemAlloc(sizeof(snet_refcount_t));
      refInfo->count = 1;
      refInfo->data = (void*) ref->data;
      refInfo->list = NULL;
      SNetRefRefcountMapSet(localRefMap, *ref, refInfo);
    }

    pthread_mutex_unlock(&localRefMutex);
  } else {
    pthread_mutex_lock(&remoteRefMutex);

    refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);
    if (--refInfo->count == 0) {
      if (refInfo->data) FREEFUN(ref->interface, refInfo->data);
      SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }
}

void SNetRefUpdate(snet_ref_t *ref, int value)
{
  pthread_mutex_lock(&localRefMutex);

  snet_refcount_t *refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
  refInfo->count += value;
  if (refInfo->count == 0) {
    FREEFUN(ref->interface, refInfo->data);
    SNetMemFree(SNetRefRefcountMapTake(localRefMap, *ref));
  }
  pthread_mutex_unlock(&localRefMutex);
}

void SNetRefSet(snet_ref_t *ref, void *data)
{
  snet_stream_t *str;
  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  LIST_DEQUEUE_EACH(refInfo->list, str) {
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'w');
    SNetStreamWrite(sd, (void*) COPYFUN(ref->interface, data));
    SNetStreamClose(sd, false);
    // Counter is updated here instead of the fetching side to avoid a race
    // across node boundaries.
    refInfo->count--;
  }

  SNetStreamListDestroy(refInfo->list);

  if (refInfo->count > 0) {
    refInfo->data = data;
  } else {
    FREEFUN(ref->interface, data);
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  }

  pthread_mutex_unlock(&remoteRefMutex);
}

void SNetRefFetch(snet_ref_t *ref, int node)
{
  SNetDistribSendData(ref, SNetRefGetData(ref), node);
  SNetRefUpdate(ref, -1);
}
