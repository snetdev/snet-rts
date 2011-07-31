#include <assert.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include "record.h"
#include "memfun.h"
#include "distribmap.h"

snet_stream_dest_map_t *streamMap;
static snet_streamset_t outgoing = NULL;

static int offset;
static size_t size = 1000;
static char buf[1000];
extern int node_location;

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
  SNetRecSerialise(rec, &MPIPackWrapper);
  //fprintf(stderr, "#%d: Sending record to: %d, %d, %d, %d\n", node_location, dest.node, dest.dest, dest.parent, dest.parentIndex);
  //fflush(stderr);
  MPI_Send(buf, offset, MPI_PACKED, dest.node, dest.parent, MPI_COMM_WORLD);
}

void SNetOutputManager(void *args)
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
      if (rec == NULL) {
        assert(SNetStreamDestMapSize(streamMap) == 0);
        return;
      }
      sd = SNetStreamOpen(COLL_REC(rec, output), 'r');
      SNetStreamDestMapRename(streamMap, COLL_REC(rec, output), sd);
      SNetStreamsetPut(&outgoing, sd);
      SNetRecDestroy(rec);
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
