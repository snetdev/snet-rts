#include <assert.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include "entities.h"
#include "record.h"
#include "memfun.h"
#include "distribmap.h"

typedef struct {
  snet_dest_t dest;
  snet_stream_t *stream;
} tmp_t;

snet_stream_dest_map_t *streamMap;
static snet_streamset_t outgoing = NULL;

static int offset;
static size_t size = 1000;
static char buf[1000];
extern int node_location;
extern bool outputDistribInfo;

static void MPIPackWrapper(int foo, int *bar)
{
  MPI_Pack(bar, foo, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
}

static void SendRecord(snet_dest_t dest, snet_record_t *rec)
{
  offset = 0;
  MPIPackWrapper(1, &dest.dest);
  MPIPackWrapper(1, &dest.parentIndex);
  MPIPackWrapper(1, &dest.parentNode);
  MPIPackWrapper(1, &dest.blaat);
  SNetRecSerialise(rec, &MPIPackWrapper);
  if (outputDistribInfo) {
    if (REC_DESCR(rec) == REC_terminate) {
      fprintf(stderr, "#%d: Sending terminate record to: %d, %d, %d, %d\n", node_location, dest.node, dest.dest, dest.parent, dest.parentIndex);
    } else {
      fprintf(stderr, "#%d: Sending record to: %d, %d, %d, %d\n", node_location, dest.node, dest.dest, dest.parent, dest.parentIndex);
    }
    fflush(stderr);
  }
  MPI_Send(buf, offset, MPI_PACKED, dest.node, dest.parent, MPI_COMM_WORLD);
}

void SNetOutputManager(snet_entity_t *ent, void *args)
{
  snet_dest_t dest;
  snet_record_t *rec;
  snet_stream_desc_t *sd;
  snet_stream_desc_t *input = SNetStreamOpen((snet_stream_t*) args, 'r');

  SNetStreamsetPut(&outgoing, input);

  while (true) {
    sd = SNetStreamPoll(&outgoing);
    rec = (snet_record_t*) SNetStreamRead(sd);
    if (sd == input) {
      tmp_t *blaat = (tmp_t*) rec;
      if (rec == NULL) {
        assert(SNetStreamDestMapSize(streamMap) == 0);
        return;
      }
      sd = SNetStreamOpen(blaat->stream, 'r');
      SNetStreamDestMapSet(streamMap, sd, blaat->dest);
      SNetStreamsetPut(&outgoing, sd);
      SNetMemFree(blaat);
    } else if (rec != NULL) {
      if (REC_DESCR(rec) == REC_sync) {
        SNetStreamReplace(sd, SNetRecGetStream(rec));
      } else {
        if (REC_DESCR(rec) == REC_terminate) {
          dest = SNetStreamDestMapTake(streamMap, sd);
          SNetStreamsetRemove(&outgoing, sd);
        } else {
          dest = SNetStreamDestMapGet(streamMap, sd);
        }
        SendRecord(dest, rec);
      }
      SNetRecDestroy(rec);
    }
  }
}
