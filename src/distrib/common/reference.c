#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "debug.h"
#include "distribcommon.h"
#include "distribution.h"
#include "interface_functions.h"
#include "memfun.h"
#include "reference.h"
#include "stream.h"
#include "threading.h"
#include "networkinterface.h"

#define COPYFUN(interface, data)    SNetInterfaceGet(interface)->copyfun(data)
#define FREEFUN(interface, data)    SNetInterfaceGet(interface)->freefun(data)

struct snet_ref {
    int         node;           /* location */
    int         interface;      /* box language interface */
    uintptr_t   data;           /* pointer to field data */
};

/* Declare a list of streams. */
#define LIST_NAME_H Stream
#define LIST_TYPE_NAME_H stream
#define LIST_VAL_H snet_stream_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

/* Declare a list of pointers to data pointers. */
#define LIST_NAME_H             Result
#define LIST_TYPE_NAME_H        result
#define LIST_VAL_H              void**
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

/* Count the number of references
 * to a data field from a remote location. */
struct snet_refcount {
  int                   count;
  void                 *data;

  /* The Threaded-Entity RTS (TERTS) uses a
   * multi-reader-single-writer list of blocking streams
   * to communicate the result of a remote fetch
   * to, potentially, multiple waiting threads. */
  snet_stream_list_t   *stream_list;

  /* The Front RTS uses a POSIX condition variable.
   * Potentially multiple waiting threads are
   * woken up by a call to pthread_cond_broadcast. */
  pthread_cond_t        cond_var;
  snet_result_list_t   *result_list;
};

#define MAP_NAME_H RefRefcount
#define MAP_TYPE_NAME_H ref_refcount
#define MAP_KEY_H snet_ref_t
#define MAP_VAL_H snet_refcount_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

/* Define a list of streams. */
#define LIST_NAME Stream
#define LIST_TYPE_NAME stream
#define LIST_VAL snet_stream_t*
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

/* Define a list of pointers to data pointers. */
#define LIST_NAME       Result
#define LIST_TYPE_NAME  result
#define LIST_VAL        void**
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

static size_t (*snet_ref_data_size)(void*) = NULL;

void SNetReferenceSetDataFunc(size_t (*get_size)(void *)) {
	snet_ref_data_size = get_size;
}

/* data: c4snet_data */
size_t SNetRefGetDataSize(void *data) {
	if (snet_ref_data_size == NULL)
		return -1;
	return snet_ref_data_size(data);
}

/* Called by toplevel distribution. */
void SNetReferenceInit(void)
{
  localRefMap = SNetRefRefcountMapCreate(0);
  remoteRefMap = SNetRefRefcountMapCreate(0);
}

/* Called by input manager. */
void SNetReferenceDestroy(void)
{
  pthread_mutex_destroy(&localRefMutex);
  pthread_mutex_destroy(&remoteRefMutex);
  assert(SNetRefRefcountMapSize(localRefMap) == 0);
  assert(SNetRefRefcountMapSize(remoteRefMap) == 0);
  SNetRefRefcountMapDestroy(localRefMap);
  SNetRefRefcountMapDestroy(remoteRefMap);
}

/* Called by input parser, or box output. */
snet_ref_t *SNetRefCreate(void *data, int interface)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));

  result->node = SNetDistribGetNodeId();
  result->interface = interface;
  result->data = (uintptr_t) data;

  return result;
}

/* Called by synchro-cells and by flow-inheritance. */
snet_ref_t *SNetRefCopy(snet_ref_t *ref)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));
  *result = *ref;

  if (SNetDistribIsNodeLocation(ref->node)) {
    result->data = (uintptr_t) COPYFUN(ref->interface, (void*)ref->data);
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

/* Called by distribution implementation layer. */
void SNetRefSerialise(snet_ref_t *ref, void *buf,
                      void (*serialiseInt)(void*, int, int*),
                      void (*serialiseByte)(void*, int, char*))
{
    serialiseInt(buf, 1, &ref->node);
    serialiseInt(buf, 1, &ref->interface);
    serialiseByte(buf, sizeof(uintptr_t), (char*) &ref->data);
}

/* Called by distribution implementation layer. */
snet_ref_t *SNetRefDeserialise(void *buf,
                               void (*deserialiseInt)(void*, int, int*),
                               void (*deserialiseByte)(void*, int, char*))
{
    snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));
    deserialiseInt(buf, 1, &result->node);
    deserialiseInt(buf, 1, &result->interface);
    deserialiseByte(buf, sizeof(uintptr_t), (char*) &result->data);
    return result;
}

/* Called by distribution implementation layer. */
int SNetRefInterface(snet_ref_t *ref)
{ return ref->interface; };

/* Called by distribution implementation layer. */
int SNetRefNode(snet_ref_t *ref)
{ return ref->node; };

/* Fetch data from a remote location: Called below by SNetRefGetData. */
static void* GetRemoteData(snet_ref_t *ref)
{
  void                  *result = NULL;
  snet_refcount_t       *refInfo;

  pthread_mutex_lock(&remoteRefMutex);

  refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);
  if (refInfo->data == NULL) {

    /* Discern between Threaded-Entity Runtime and Front. */
    int runtime = SNetRuntimeGet();
    switch (runtime) {

      /* Original implementation which exploits blocking streams. */
      case Streams: {
          snet_stream_t *str = SNetStreamCreate(1);
          snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

          if (refInfo->stream_list) {
            SNetStreamListAppendEnd(refInfo->stream_list, str);
          } else {
            refInfo->stream_list = SNetStreamListCreate(1, str);
            SNetDistribFetchRef(ref);
          }

          // refInfo->count gets updated by SNetRefSet instead of here
          // to avoid a race between nodes when a pointer is recycled.
          pthread_mutex_unlock(&remoteRefMutex);
          result = SNetStreamRead(sd);
          SNetStreamClose(sd, true);
        }
        break;

      /* New implementation in case streams are non-blocking. */
      case Front: {
          if (refInfo->result_list == NULL) {
            refInfo->result_list = SNetResultListCreate(1, &result);
            SNetDistribFetchRef(ref);
          } else {
            SNetResultListAppendEnd(refInfo->result_list, &result);
          }
          pthread_cond_wait(&refInfo->cond_var, &remoteRefMutex);
          pthread_mutex_unlock(&remoteRefMutex);
          assert(result != NULL);
        }
        break;

      /* Impossible cases. */
      default: SNetUtilDebugFatal("[%s]: rt %d", __func__, runtime);
        break;
    }

  } else {

    SNetDistribUpdateRef(ref, -1);
    if (--refInfo->count == 0) {
      result = refInfo->data;
      refInfo->data = NULL;
      pthread_cond_destroy(&refInfo->cond_var);
      SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
    } else {
      result = COPYFUN(ref->interface, refInfo->data);
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }

  return result;
}

/* Called by distribution implementation layer,
 *        by input manager, by output entity, by observer. */
void *SNetRefGetData(snet_ref_t *ref)
{
  if (SNetDistribIsNodeLocation(ref->node)) {
    /* Data is already local: nothing to be done. */
  } else {
    /* Fetch the referenced data from a remote location. */
    ref->data = (uintptr_t) GetRemoteData(ref);
    ref->node = SNetDistribGetNodeId();
  }
  return (void*) ref->data;
}

/* Called just before invoking a box. */
void *SNetRefTakeData(snet_ref_t *ref)
{
  void *result = SNetRefGetData(ref);
  SNetMemFree(ref);
  return result;
}

/* Called by SNetRecDestroy. */
void SNetRefDestroy(snet_ref_t *ref)
{
  if (SNetDistribIsNodeLocation(ref->node)) {
    FREEFUN(ref->interface, (void*)ref->data);
    SNetMemFree(ref);
    return;
  }

  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  SNetDistribUpdateRef(ref, -1);
  if (--refInfo->count == 0) {
    if (refInfo->data) FREEFUN(ref->interface, refInfo->data);
    refInfo->data = NULL;
    pthread_cond_destroy(&refInfo->cond_var);
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  }
  pthread_mutex_unlock(&remoteRefMutex);

  SNetMemFree(ref);
}

/* Called by distribution implementation layer,
 * upon deserialisation of an incoming record. */
void SNetRefIncoming(snet_ref_t *ref)
{
  snet_refcount_t *refInfo;

  if (SNetDistribIsNodeLocation(ref->node)) {
    pthread_mutex_lock(&localRefMutex);

    refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
    if (--refInfo->count == 0) {
      refInfo->data = NULL;
      pthread_cond_destroy(&refInfo->cond_var);
      SNetMemFree(SNetRefRefcountMapTake(localRefMap, *ref));
    } else {
      ref->data = (uintptr_t) COPYFUN(ref->interface, (void*)ref->data);
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
      refInfo->data = NULL;
      refInfo->stream_list = NULL;
      pthread_cond_init(&refInfo->cond_var, NULL);
      refInfo->result_list = NULL;
      SNetRefRefcountMapSet(remoteRefMap, *ref, refInfo);
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }
}

/* Called by distribution implementation layer. */
void SNetRefOutgoing(snet_ref_t *ref)
{
  snet_refcount_t *refInfo;

  if (SNetDistribIsNodeLocation(ref->node)) {
    pthread_mutex_lock(&localRefMutex);

    if (SNetRefRefcountMapContains(localRefMap, *ref)) {
      refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
      refInfo->count++;
      FREEFUN(ref->interface, (void*)ref->data);
    } else {
      refInfo = SNetMemAlloc(sizeof(snet_refcount_t));
      refInfo->count = 1;
      refInfo->data = (void*) ref->data;
      refInfo->stream_list = NULL;
      pthread_cond_init(&refInfo->cond_var, NULL);
      refInfo->result_list = NULL;
      SNetRefRefcountMapSet(localRefMap, *ref, refInfo);
    }

    pthread_mutex_unlock(&localRefMutex);
  } else {
    pthread_mutex_lock(&remoteRefMutex);

    refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);
    if (--refInfo->count == 0) {
      if (refInfo->data) FREEFUN(ref->interface, refInfo->data);
      refInfo->data = NULL;
      pthread_cond_destroy(&refInfo->cond_var);
      SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
    }

    pthread_mutex_unlock(&remoteRefMutex);
  }
}

/* Called by input manager. */
void SNetRefUpdate(snet_ref_t *ref, int value)
{
  pthread_mutex_lock(&localRefMutex);

  snet_refcount_t *refInfo = SNetRefRefcountMapGet(localRefMap, *ref);
  refInfo->count += value;
  if (refInfo->count == 0) {
    FREEFUN(ref->interface, refInfo->data);
    refInfo->data = NULL;
    pthread_cond_destroy(&refInfo->cond_var);
    SNetMemFree(SNetRefRefcountMapTake(localRefMap, *ref));
  }
  pthread_mutex_unlock(&localRefMutex);
}

/* Called by input manager. */
void SNetRefSet(snet_ref_t *ref, void *data)
{
  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  /* Discern between Threaded-Entity runtime and Front. */
  int runtime = SNetRuntimeGet();
  switch (runtime) {

    /* Threaded-Entity RTS: exploiting that streams are blocking. */
    case Streams: {
        snet_stream_t *str;

        LIST_DEQUEUE_EACH(refInfo->stream_list, str) {
          snet_stream_desc_t *sd = SNetStreamOpen(str, 'w');
          SNetStreamWrite(sd, COPYFUN(ref->interface, data));
          SNetStreamClose(sd, false);
          // Counter is updated here, instead of the fetching side,
          // to avoid a race across node boundaries.
          refInfo->count--;
        }

        SNetStreamListDestroy(refInfo->stream_list);
        refInfo->stream_list = NULL;
      }
      break;

    /* The Front runtime system has non-blocking streams. */
    case Front: {
        void **result_ptr;

        LIST_DEQUEUE_EACH(refInfo->result_list, result_ptr) {
          *result_ptr = COPYFUN(ref->interface, data);
          // Counter is updated here, instead of the fetching side,
          // to avoid a race across node boundaries.
          refInfo->count--;
        }
        SNetResultListDestroy(refInfo->result_list);
        refInfo->result_list = NULL;
        pthread_cond_broadcast(&refInfo->cond_var);
      }
      break;

    default: SNetUtilDebugFatal("[%s]: rt %d\n", __func__, runtime);
      break;
  }

  if (refInfo->count > 0) {
    refInfo->data = data;
  } else {
    FREEFUN(ref->interface, data);
    refInfo->data = NULL;
    pthread_cond_destroy(&refInfo->cond_var);
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  }

  pthread_mutex_unlock(&remoteRefMutex);
}

