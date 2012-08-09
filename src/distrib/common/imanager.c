#include <assert.h>
#include <pthread.h>

#include "debug.h"
#include "distribcommon.h"
#include "imanager.h"
#include "memfun.h"
#include "omanager.h"
#include "reference.h"
#include "tuplelist.h"

#define MAP_NAME_H DestStream
#define MAP_TYPE_NAME_H dest_stream
#define MAP_KEY_H snet_dest_t
#define MAP_VAL_H snet_stream_desc_t*
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#define LIST_NAME_H Record
#define LIST_TYPE_NAME_H record
#define LIST_VAL_H snet_record_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define LIST_NAME_H Buffer
#define LIST_TYPE_NAME_H buffer
#define LIST_VAL_H snet_buffer_t*
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define MAP_NAME DestStream
#define MAP_TYPE_NAME dest_stream
#define MAP_KEY snet_dest_t
#define MAP_VAL snet_stream_desc_t*
#define MAP_KEY_CMP SNetDestCompare
#include "map-template.c"
#undef MAP_KEY_FREE_FUN
#undef MAP_KEY_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

#define LIST_NAME Record
#define LIST_TYPE_NAME record
#define LIST_VAL snet_record_t*
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

#define LIST_NAME Buffer
#define LIST_TYPE_NAME buffer
#define LIST_VAL snet_buffer_t*
#include "list-template.c"
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME


void SNetRouteNewDynamic(snet_dest_t dest);

struct snet_buffer {
  bool done;
  unsigned int counter;
  pthread_mutex_t mutex;
  snet_dest_t dest;
  snet_record_list_t *list;
  snet_stream_desc_t *sd;
};

static snet_tuple_list_t *newStreams = NULL;
static snet_buffer_list_t *unblocked = NULL;
static pthread_mutex_t unblockedMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t newStreamsMutex = PTHREAD_MUTEX_INITIALIZER;

static void ReadCallback(snet_buffer_t *buf)
{
  pthread_mutex_lock(&buf->mutex);

  if (--buf->counter == 0) {
    if (SNetRecordListLength(buf->list) > 0) {
      pthread_mutex_lock(&unblockedMutex);

      SNetBufferListAppendEnd(unblocked, buf);
      if (SNetBufferListLength(unblocked) == 1) SNetDistribUpdateBlocked();

      pthread_mutex_unlock(&unblockedMutex);
    } else if (buf->done) {
      pthread_mutex_unlock(&buf->mutex);
      pthread_mutex_destroy(&buf->mutex);

      SNetStreamClose(buf->sd, false);
      SNetRecordListDestroy(buf->list);
      SNetMemFree(buf);
      return;
    }
  }

  pthread_mutex_unlock(&buf->mutex);
}

static void UpdateBlocked(void)
{
  snet_buffer_t *buf;
  snet_record_t *rec;

  pthread_mutex_lock(&unblockedMutex);
  LIST_DEQUEUE_EACH(unblocked, buf) {
    pthread_mutex_lock(&buf->mutex);
    LIST_DEQUEUE_EACH(buf->list, rec) {
      if (SNetStreamTryWrite(buf->sd, rec) == 0) {
        buf->counter++;
      } else {
        SNetRecordListAppendStart(buf->list, rec);
        break;
      }
    }

    if (!SNetRecordListLength(buf->list) && !buf->done) {
      SNetDistribUnblockDest(buf->dest);
    }
    pthread_mutex_unlock(&buf->mutex);
  }
  pthread_mutex_unlock(&unblockedMutex);
}

static void UpdateIncoming(snet_dest_stream_map_t *destMap)
{
  snet_tuple_t tuple;

  pthread_mutex_lock(&newStreamsMutex);
  LIST_DEQUEUE_EACH(newStreams, tuple) {
    snet_buffer_t *buf = SNetMemAlloc(sizeof(snet_buffer_t));
    pthread_mutex_init(&buf->mutex, NULL);
    buf->counter = 0;
    buf->done = false;
    buf->dest = tuple.dest;
    buf->list = SNetRecordListCreate(0);
    buf->sd = SNetStreamOpen(tuple.stream, 'w');

    SNetStreamRegisterReadCallback(tuple.stream, (void (*)(void*)) &ReadCallback, buf);
    SNetDestStreamMapSet(destMap, tuple.dest, buf->sd);
  }
  pthread_mutex_unlock(&newStreamsMutex);
}

static void HandleRecord(snet_dest_stream_map_t *destMap, snet_record_t *rec,
                         snet_dest_t dest)
{
  snet_buffer_t *buf;
  snet_stream_desc_t *sd;

  UpdateIncoming(destMap);
  if (!SNetDestStreamMapContains(destMap, dest)) {
    SNetRouteNewDynamic(dest);
    UpdateIncoming(destMap);
  }

  if (SNetRecGetDescriptor(rec) == REC_terminate) {
    sd = SNetDestStreamMapTake(destMap, dest);
  } else {
    sd = SNetDestStreamMapGet(destMap, dest);
  }

  buf = SNetStreamGetCallbackArg(sd);
  pthread_mutex_lock(&buf->mutex);
  if (SNetRecGetDescriptor(rec) == REC_terminate) buf->done = true;

  if (!SNetRecordListLength(buf->list) && SNetStreamTryWrite(sd, rec) == 0) {
    buf->counter++;
  } else {
    SNetRecordListAppendEnd(buf->list, rec);
    if (SNetRecordListLength(buf->list) == 1) SNetDistribBlockDest(dest);
  }
  pthread_mutex_unlock(&buf->mutex);
}



void SNetInputManager( void *args);

void SNetInputManagerNewIn(snet_dest_t dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};
  pthread_mutex_lock(&newStreamsMutex);
  SNetTupleListAppendEnd(newStreams, tuple);
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetInputManagerInit(void)
{
  newStreams = SNetTupleListCreate(0);
  unblocked = SNetBufferListCreate(0);
}

void SNetInputManagerStart(void)
{
  SNetThreadingSpawn( ENTITY_other, -1, NULL,
      "input_manager", &SNetInputManager, NULL);
}

void SNetInputManager(void *args)
{
  (void) args; /* NOT USED */
  snet_dest_stream_map_t *destMap = SNetDestStreamMapCreate(0);

  while (true) {
    snet_msg_t msg = SNetDistribRecvMsg();
    switch (msg.type) {
      case snet_rec:
        HandleRecord(destMap, msg.rec, msg.dest);
        break;

      case snet_update:
        UpdateBlocked();
        break;

      case snet_block:
        SNetOutputManagerBlock(msg.dest);
        break;

      case snet_unblock:
        SNetOutputManagerUnblock(msg.dest);
        break;

      case snet_ref_set:
        SNetRefSet(msg.ref, (void*) msg.data);
        SNetMemFree(msg.ref);
        break;

      case snet_ref_update:
        SNetRefUpdate(msg.ref, msg.val);
        SNetMemFree(msg.ref);
        break;

      case snet_ref_fetch:
        SNetDistribSendData(msg.ref, SNetRefGetData(msg.ref), (void*) msg.data);
        SNetRefUpdate(msg.ref, -1);
        SNetMemFree(msg.ref);
        break;

      case snet_stop:
        goto exit;

      default:
        SNetUtilDebugFatal("Unknown SNet distribution message.");
        break;
    }
  }

exit:
  assert(SNetDestStreamMapSize(destMap) == 0);
  SNetDestStreamMapDestroy(destMap);
  SNetOutputManagerStop();
  SNetReferenceDestroy();
  pthread_mutex_destroy(&newStreamsMutex);
  pthread_mutex_destroy(&unblockedMutex);
  SNetDistribStop();
  return;
}
