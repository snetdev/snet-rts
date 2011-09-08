#include <assert.h>
#include <pthread.h>

#include "distribcollections.h"
#include "entities.h"
#include "iomanagers.h"
#include "memfun.h"
#include "omanager.h"
#include "reference.h"
#include "tuple.h"

struct snet_buffer {
  unsigned int total;
  bool done;
  pthread_mutex_t mutex;
  snet_dest_t *dest;
  snet_record_list_t *list;
};

static snet_tuple_list_t *newStreams = NULL;
static snet_buffer_list_t *unblocked = NULL;
static snet_dest_stream_map_t *destMap = NULL;
static pthread_mutex_t newStreamsMutex = PTHREAD_MUTEX_INITIALIZER;

static void ReadCallback(void *arg)
{
  bool cleanup;
  snet_buffer_t *buf = arg;

  pthread_mutex_lock(&buf->mutex);
  cleanup = --buf->total == 0 && buf->done;

  if (SNetRecordListLength(buf->list) > 0) {
    pthread_mutex_lock(&newStreamsMutex);
    SNetBufferListAppendEnd(unblocked, arg);
    pthread_mutex_unlock(&newStreamsMutex);
    SNetDistribRemoteDestUnblock(NULL);
  }
  pthread_mutex_unlock(&buf->mutex);

  if (cleanup) {
    pthread_mutex_lock(&newStreamsMutex);
    SNetStreamClose(SNetDestStreamMapTake(destMap, buf->dest), false);
    pthread_mutex_unlock(&newStreamsMutex);

    SNetRecordListDestroy(buf->list);
    pthread_mutex_destroy(&buf->mutex);
    SNetMemFree(buf);
  }
}

void SNetInputManagerNewIn(snet_dest_t *dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};
  pthread_mutex_lock(&newStreamsMutex);
  SNetTupleListAppendEnd(newStreams, tuple);
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetInputManagerUpdate(void)
{
  snet_record_t *rec;
  snet_buffer_t *buf;
  snet_tuple_t tuple;

  pthread_mutex_lock(&newStreamsMutex);
  LIST_DEQUEUE_EACH(newStreams, tuple) {
    snet_buffer_t *buf = SNetMemAlloc(sizeof(snet_buffer_t));
    pthread_mutex_init(&buf->mutex, NULL);
    buf->total = 0;
    buf->done = false;
    buf->list = SNetRecordListCreate(0);
    buf->dest = tuple.dest;
    assert(buf->list != NULL);

    SNetStreamRegisterReadCallback(tuple.stream, &ReadCallback, buf);
    SNetDestStreamMapSet(destMap, tuple.dest, SNetStreamOpen(tuple.stream, 'w'));
  }

  LIST_DEQUEUE_EACH(unblocked, buf) {
    pthread_mutex_lock(&buf->mutex);
    snet_stream_desc_t *sd = SNetDestStreamMapGet(destMap, buf->dest);
    LIST_DEQUEUE_EACH(buf->list, rec) {
      if (SNetStreamTryWrite(sd, rec) != 0) {
        SNetRecordListAppendStart(buf->list, rec);
        break;
      }
    }

    if (SNetRecordListLength(buf->list) == 0 && !buf->done) {
        SNetDistribRemoteDestUnblock(buf->dest);
    }
    pthread_mutex_unlock(&buf->mutex);
  }
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetInputManager(snet_entity_t *ent, void *args);

void SNetInputManagerInit(void)
{
  newStreams = SNetTupleListCreate(0);
  unblocked = SNetBufferListCreate(0);
  destMap = SNetDestStreamMapCreate(0);
}

void SNetInputManagerStart(void)
{
  SNetThreadingSpawn(
    SNetEntityCreate( ENTITY_other, -1, NULL,
      "input_manager", &SNetInputManager, NULL));
}

void SNetInputManager(snet_entity_t *ent, void *args)
{
  bool destExists;
  snet_stream_desc_t *sd;
  (void) ent; /* NOT USED */
  (void) args; /* NOT USED */

  while (true) {
    snet_dest_t *dest;
    snet_record_t *rec = SNetDistribRecvRecord(&dest);
    if (rec == NULL) break;

    SNetInputManagerUpdate();

    pthread_mutex_lock(&newStreamsMutex);
    destExists = SNetDestStreamMapContains(destMap, dest);
    pthread_mutex_unlock(&newStreamsMutex);

    if (!destExists) {
      SNetDistribNewDynamicCon(dest);
      SNetInputManagerUpdate();
    }

    pthread_mutex_lock(&newStreamsMutex);
    sd = SNetDestStreamMapGet(destMap, dest);
    pthread_mutex_unlock(&newStreamsMutex);

    snet_buffer_t *buf = SNetStreamGetCallbackArg(sd);
    pthread_mutex_lock(&buf->mutex);
    buf->total++;
    if (SNetRecGetDescriptor(rec) == REC_terminate) buf->done = true;

    if (SNetStreamTryWrite(sd, rec) == -1) {
      SNetRecordListAppendEnd(buf->list, rec);
      SNetDistribRemoteDestBlock(dest);
    }
    pthread_mutex_unlock(&buf->mutex);
  }

  assert(SNetDestStreamMapSize(destMap) == 0);
  SNetOutputManagerStop();
  SNetReferenceDestroy();
  pthread_mutex_destroy(&newStreamsMutex);
  SNetDistribStop(false);
  return;
}
