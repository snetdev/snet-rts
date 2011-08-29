#include <assert.h>
#include <pthread.h>

#include "distribcollections.h"
#include "entities.h"
#include "iomanagers.h"
#include "memfun.h"

static snet_tuple_list_t *newStreams = NULL;
static pthread_mutex_t newStreamsMutex = PTHREAD_MUTEX_INITIALIZER;

void SNetDistribNewIn(snet_dest_t *dest, snet_stream_t *stream)
{
  snet_tuple_t tuple = {dest, stream};

  pthread_mutex_lock(&newStreamsMutex);

  if (newStreams == NULL) newStreams = SNetTupleListCreate(0);
  SNetTupleListAppendEnd(newStreams, tuple);

  pthread_mutex_unlock(&newStreamsMutex);
}

static void UpdateMap(snet_dest_stream_map_t *map)
{
  pthread_mutex_lock(&newStreamsMutex);
  while (SNetTupleListLength(newStreams) != 0) {
    snet_tuple_t tuple = SNetTupleListPopStart(newStreams);
    SNetDestStreamMapSet(map, tuple.dest, SNetStreamOpen(tuple.stream, 'w'));
  }
  pthread_mutex_unlock(&newStreamsMutex);
}

void SNetInputManager(snet_entity_t *ent, void *args)
{
  snet_dest_stream_map_t *destMap = SNetDestStreamMapCreate(0);

  pthread_mutex_lock(&newStreamsMutex);
  if (newStreams == NULL) newStreams = SNetTupleListCreate(0);
  pthread_mutex_unlock(&newStreamsMutex);

  while (true) {
    snet_dest_t *dest;
    snet_record_t *rec = RecvRecord(&dest);

    if (rec == NULL) break;

    UpdateMap(destMap);

    if (!SNetDestStreamMapContains(destMap, dest)) {
      SNetDistribNewDynamicCon(dest);
    }

    UpdateMap(destMap);

    if (SNetRecGetDescriptor(rec) == REC_terminate) {
      snet_stream_desc_t *sd = SNetDestStreamMapTake(destMap, dest);
      SNetStreamWrite(sd, rec);
      SNetStreamClose(sd, false);
    } else {
      SNetStreamWrite(SNetDestStreamMapGet(destMap, dest), rec);
    }
  }

  assert(SNetDestStreamMapSize(destMap) == 0);
  SNetDistribStopOutputManager();
  pthread_mutex_destroy(&newStreamsMutex);
  SNetDistribStop(false);
  return;
}
