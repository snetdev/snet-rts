#include <assert.h>
#include <pthread.h>

#include "distribcommon.h"
#include "memfun.h"
#include "omanager.h"
#include "record.h"
#include "tuplelist.h"

#define MAP_NAME_H StreamDest
#define MAP_TYPE_NAME_H stream_dest
#define MAP_KEY_H snet_stream_desc_t*
#define MAP_VAL_H snet_dest_t
#include "map-template.h"
#undef MAP_VAL_H
#undef MAP_KEY_H
#undef MAP_TYPE_NAME_H
#undef MAP_NAME_H

#define LIST_NAME_H Dest
#define LIST_TYPE_NAME_H dest
#define LIST_VAL_H snet_dest_t
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#define MAP_NAME StreamDest
#define MAP_TYPE_NAME stream_dest
#define MAP_KEY snet_stream_desc_t*
#define MAP_VAL snet_dest_t
#define MAP_VAL_CMP SNetDestCompare
#include "map-template.c"
#undef MAP_VAL_CMP
#undef MAP_VAL
#undef MAP_KEY
#undef MAP_TYPE_NAME
#undef MAP_NAME

#define LIST_NAME Dest
#define LIST_TYPE_NAME dest
#define LIST_VAL snet_dest_t
#define LIST_CMP SNetDestCompare
#include "list-template.c"
#undef LIST_CMP
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME

static bool running = true;
static snet_stream_t *wakeupStream = NULL;
static snet_tuple_list_t *newStreams = NULL;
static snet_dest_list_t *newBlocked = NULL;
static snet_dest_list_t *newUnblocked = NULL;
static pthread_mutex_t outputManagerMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetOutputManagerNewOut(snet_dest_t dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};

  pthread_mutex_lock(&outputManagerMutex);

  SNetTupleListAppendEnd(newStreams, tuple);
  if (wakeupStream != NULL) {
    snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
    SNetStreamTryWrite(sd, (void*)1);
    SNetStreamClose(sd, false);
  }

  pthread_mutex_unlock(&outputManagerMutex);
}

static void UpdateState(snet_dest_t dest, bool block)
{
  pthread_mutex_lock(&outputManagerMutex);
  SNetDestListAppendEnd(block ? newBlocked : newUnblocked, dest);
  snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
  SNetStreamTryWrite(sd, (void*)1);
  SNetStreamClose(sd, false);
  pthread_mutex_unlock(&outputManagerMutex);
}

void SNetOutputManagerBlock(snet_dest_t dest) { UpdateState(dest, true); }
void SNetOutputManagerUnblock(snet_dest_t dest) { UpdateState(dest, false); }

static void UpdateOutgoing(snet_stream_dest_map_t *map, snet_streamset_t *set)
{
  snet_tuple_t tuple;
  snet_stream_desc_t *sd;

  pthread_mutex_lock(&outputManagerMutex);
  LIST_DEQUEUE_EACH(newStreams, tuple) {
    sd = SNetStreamOpen(tuple.stream, 'r');
    SNetStreamDestMapSet(map, sd, tuple.dest);
    SNetStreamsetPut(set, sd);
  }
  pthread_mutex_unlock(&outputManagerMutex);
}

static void UpdateBlocked(snet_stream_dest_map_t *map, snet_dest_list_t *list,
                          snet_streamset_t *oldSet, snet_streamset_t *newSet)
{
  snet_dest_t dest;
  snet_stream_desc_t *sd;

  pthread_mutex_lock(&outputManagerMutex);
  LIST_DEQUEUE_EACH(list, dest) {
    if ((sd = SNetStreamDestMapFindVal(map, dest, NULL)) != NULL) {
      SNetStreamsetRemove(oldSet, sd);
      SNetStreamsetPut(newSet, sd);
    }
  }
  pthread_mutex_unlock(&outputManagerMutex);
}

void SNetOutputManager(void *args);

void SNetOutputManagerInit(void)
{
  newStreams = SNetTupleListCreate(0);
  newBlocked = SNetDestListCreate(0);
  newUnblocked = SNetDestListCreate(0);
}

void SNetOutputManagerStart(void)
{
  wakeupStream = SNetStreamCreate(1);
  SNetThreadingSpawn( ENTITY_other, -1, NULL,
      "output_manager", &SNetOutputManager, NULL);
}

void SNetOutputManagerStop(void)
{
  pthread_mutex_lock(&outputManagerMutex);

  running = false;
  snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
  SNetStreamTryWrite(sd, (void*)1);
  SNetStreamClose(sd, false);

  pthread_mutex_unlock(&outputManagerMutex);
}

void SNetOutputManager(void *args)
{
  (void) args; /* NOT USED */
  snet_streamset_t waiting = NULL;
  snet_streamset_t blocked = NULL;
  snet_stream_dest_map_t *streamMap = SNetStreamDestMapCreate(0);
  snet_stream_desc_t *wakeupDesc = SNetStreamOpen(wakeupStream, 'r');

  SNetStreamsetPut(&waiting, wakeupDesc);

  UpdateOutgoing(streamMap, &waiting);
  while (running) {
    snet_dest_t dest;
    snet_record_t *rec;
    snet_stream_desc_t *sd = SNetStreamPoll(&waiting);

    if (sd == wakeupDesc) {
      SNetStreamRead(sd);
      UpdateOutgoing(streamMap, &waiting);
      UpdateBlocked(streamMap, newBlocked, &waiting, &blocked);
      UpdateBlocked(streamMap, newUnblocked, &blocked, &waiting);
      continue;
    }

    rec = SNetStreamRead(sd);
    switch (SNetRecGetDescriptor(rec)) {
      case REC_sync:
        SNetStreamReplace(sd, SNetRecGetStream(rec));
        SNetRecDestroy(rec);
        break;
      case REC_terminate:
        dest = SNetStreamDestMapTake(streamMap, sd);
        SNetStreamsetRemove(&waiting, sd);
        SNetDistribSendRecord(dest, rec);
        SNetStreamClose(sd, true);
        break;
      default:
        SNetDistribSendRecord(SNetStreamDestMapGet(streamMap, sd), rec);
        break;
    }
  }

  pthread_mutex_destroy(&outputManagerMutex);

  SNetStreamsetRemove(&waiting, wakeupDesc);
  SNetStreamClose(wakeupDesc, true);

  assert(SNetStreamDestMapSize(streamMap) == 0);
  SNetStreamDestMapDestroy(streamMap);
  assert(waiting == NULL);
  assert(blocked == NULL);
  return;
}
