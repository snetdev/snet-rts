#include <assert.h>
#include <pthread.h>

#include "distribcollections.h"
#include "entities.h"
#include "iomanagers.h"
#include "memfun.h"
#include "record.h"

static bool running = true;
static snet_stream_t *wakeupStream = NULL;
static snet_tuple_list_t *newStreams = NULL;
static pthread_mutex_t newStreamsMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribNewOut(snet_dest_t *dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};

  pthread_mutex_lock(&newStreamsMutex);

  if (newStreams == NULL) newStreams = SNetTupleListCreate(0);

  SNetTupleListAppendEnd(newStreams, tuple);
  if (wakeupStream != NULL) {
    snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
    SNetStreamTryWrite(sd, (void*)1);
    SNetStreamClose(sd, false);
  }

  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetDistribStopOutputManager()
{
  pthread_mutex_lock(&newStreamsMutex);

  running = false;
  if (wakeupStream != NULL) {
    snet_stream_desc_t *sd = SNetStreamOpen(wakeupStream, 'w');
    SNetStreamTryWrite(sd, NULL);
    SNetStreamClose(sd, false);
  }

  pthread_mutex_unlock(&newStreamsMutex);
}

static void UpdateMap(snet_stream_dest_map_t *map, snet_streamset_t *set)
{
  pthread_mutex_lock(&newStreamsMutex);

  while (SNetTupleListLength(newStreams) != 0) {
    snet_tuple_t tuple = SNetTupleListPopStart(newStreams);
    snet_stream_desc_t *sd = SNetStreamOpen(tuple.stream, 'r');

    SNetStreamDestMapSet(map, sd, tuple.dest);
    SNetStreamsetPut(set, sd);
  }

  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetOutputManager(snet_entity_t *ent, void *args)
{
  snet_stream_desc_t *wakeupDesc;
  snet_streamset_t outgoing = NULL;
  snet_stream_dest_map_t *streamMap = SNetStreamDestMapCreate(0);

  wakeupStream = SNetStreamCreate(1);
  wakeupDesc = SNetStreamOpen(wakeupStream, 'r');

  SNetStreamsetPut(&outgoing, wakeupDesc);

  pthread_mutex_lock(&newStreamsMutex);
  if (newStreams == NULL) newStreams = SNetTupleListCreate(0);
  pthread_mutex_unlock(&newStreamsMutex);

  UpdateMap(streamMap, &outgoing);
  while (running) {
    snet_dest_t *dest;
    snet_record_t *rec;
    snet_stream_desc_t *sd = SNetStreamPoll(&outgoing);

    if (sd == wakeupDesc) {
      SNetStreamRead(sd);
      UpdateMap(streamMap, &outgoing);
      continue;
    }

    rec = SNetStreamRead(sd);
    switch (SNetRecGetDescriptor(rec)) {
      case REC_sync:
        SNetStreamReplace(sd, SNetRecGetStream(rec));
        break;
      case REC_terminate:
        dest = SNetStreamDestMapTake(streamMap, sd);
        SNetStreamsetRemove(&outgoing, sd);
        SendRecord(dest, rec);
        SNetDestFree(dest);
        break;
      default:
        SendRecord(SNetStreamDestMapGet(streamMap, sd), rec);
        break;
    }

    SNetRecDestroy(rec);
  }

  pthread_mutex_destroy(&newStreamsMutex);

  SNetStreamClose(wakeupDesc, true);
  SNetStreamsetRemove(&outgoing, wakeupDesc);

  assert(SNetStreamDestMapSize(streamMap) == 0);
  assert(outgoing == NULL);
  return;
}
