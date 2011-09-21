#include <assert.h>
#include <pthread.h>

#include "distribcollections.h"
#include "entities.h"
#include "iomanagers.h"
#include "memfun.h"
#include "record.h"
#include "tuple.h"

static bool running = true;
static snet_stream_t *wakeupStream = NULL;
static snet_tuple_list_t *newStreams = NULL;
static snet_dest_list_t *newBlocked = NULL;
static snet_dest_list_t *newUnblocked = NULL;
static pthread_mutex_t newStreamsMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetOutputManagerNewOut(snet_dest_t *dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};

  pthread_mutex_lock(&newStreamsMutex);

  SNetTupleListAppendEnd(newStreams, tuple);
  if (wakeupStream != NULL) {
    snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
    SNetStreamTryWrite(sd, (void*)1);
    SNetStreamClose(sd, false);
  }

  pthread_mutex_unlock(&newStreamsMutex);
}

static void block(snet_dest_t *dest, bool on)
{
  pthread_mutex_lock(&newStreamsMutex);
  SNetDestListAppendEnd(on ? newBlocked : newUnblocked, dest);
  snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
  SNetStreamTryWrite(sd, (void*)1);
  SNetStreamClose(sd, false);
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetOutputManagerBlock(snet_dest_t *dest)
{ block(dest, true); }

void SNetOutputManagerUnblock(snet_dest_t *dest)
{ block(dest, false); }

static void UpdateDests(snet_stream_dest_map_t *map, snet_streamset_t *waitSet,
                        snet_streamset_t *blockSet)
{
  snet_dest_t *dest;
  snet_tuple_t tuple;
  snet_stream_desc_t *sd;

  pthread_mutex_lock(&newStreamsMutex);
  LIST_DEQUEUE_EACH(newStreams, tuple) {
    sd = SNetStreamOpen(tuple.stream, 'r');
    SNetStreamDestMapSet(map, sd, tuple.dest);
    SNetStreamsetPut(waitSet, sd);
  }

  LIST_DEQUEUE_EACH(newBlocked, dest) {
    if ((sd = SNetStreamDestMapFindVal(map, dest, NULL)) != NULL) {
      SNetStreamsetRemove(waitSet, sd);
      SNetStreamsetPut(blockSet, sd);
    }
    SNetDestFree(dest);
  }

  LIST_DEQUEUE_EACH(newUnblocked, dest) {
    if ((sd = SNetStreamDestMapFindVal(map, dest, NULL)) != NULL) {
      SNetStreamsetRemove(blockSet, sd);
      SNetStreamsetPut(waitSet, sd);
    }
    SNetDestFree(dest);
  }
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetOutputManager(snet_entity_t *ent, void *args);

void SNetOutputManagerInit(void)
{
  newStreams = SNetTupleListCreate(0);
  newBlocked = SNetDestListCreate(0);
  newUnblocked = SNetDestListCreate(0);
}

void SNetOutputManagerStart(void)
{
  wakeupStream = SNetStreamCreate(1);
  SNetThreadingSpawn(
    SNetEntityCreate( ENTITY_other, -1, NULL,
      "output_manager", &SNetOutputManager, NULL));
}

void SNetOutputManagerStop(void)
{
  pthread_mutex_lock(&newStreamsMutex);

  running = false;
  snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
  SNetStreamTryWrite(sd, (void*)1);
  SNetStreamClose(sd, false);

  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetOutputManager(snet_entity_t *ent, void *args)
{
  snet_streamset_t waiting = NULL;
  snet_streamset_t blocked = NULL;
  snet_stream_dest_map_t *streamMap = SNetStreamDestMapCreate(0);
  snet_stream_desc_t *wakeupDesc = SNetStreamOpen(wakeupStream, 'r');
  (void) ent; /* NOT USED */
  (void) args; /* NOT USED */

  SNetStreamsetPut(&waiting, wakeupDesc);

  UpdateDests(streamMap, &waiting, &blocked);
  while (running) {
    snet_dest_t *dest;
    snet_record_t *rec;
    snet_stream_desc_t *sd = SNetStreamPoll(&waiting);

    if (sd == wakeupDesc) {
      SNetStreamRead(sd);
      UpdateDests(streamMap, &waiting, &blocked);
      continue;
    }

    rec = SNetStreamRead(sd);
    switch (SNetRecGetDescriptor(rec)) {
      case REC_sync:
        SNetStreamReplace(sd, SNetRecGetStream(rec));
        break;
      case REC_terminate:
        dest = SNetStreamDestMapTake(streamMap, sd);
        SNetStreamsetRemove(&waiting, sd);
        SNetDistribSendRecord(dest, rec);
        SNetStreamClose(sd, true);
        SNetDestFree(dest);
        break;
      default:
        SNetDistribSendRecord(SNetStreamDestMapGet(streamMap, sd), rec);
        break;
    }
  }

  pthread_mutex_destroy(&newStreamsMutex);

  SNetStreamsetRemove(&waiting, wakeupDesc);
  SNetStreamClose(wakeupDesc, true);

  assert(SNetStreamDestMapSize(streamMap) == 0);
  assert(waiting == NULL);
  assert(blocked == NULL);
  return;
}
