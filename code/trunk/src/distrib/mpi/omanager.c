#include <mpi.h>
#include <pthread.h>
#include "record.h"
#include "memfun.h"
#include "distribmap.h"

extern snet_stream_dest_map_t *streamMap;

static bool running = true;
snet_streamset_t outgoing;

static void SendRecord(snet_dest_t dest, snet_record_t *rec)
{
  int tmp[2];
  int offset = 0;
  size_t size = 1000;
  char *buf = SNetMemAlloc(size);

  MPI_Pack(&DATA_REC(rec, tags)->size, 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, tags)->used, 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, btags)->size, 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, btags)->used, 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, interface_id), 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  tmp[0] = DATA_REC(rec, mode);

  MPI_Pack(tmp, 1, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);

  MPI_Pack(&DATA_REC(rec, tags)->keys, DATA_REC(rec, tags)->size, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, tags)->values, DATA_REC(rec, tags)->size, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);

  MPI_Pack(&DATA_REC(rec, btags)->keys, DATA_REC(rec, btags)->size, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
  MPI_Pack(&DATA_REC(rec, btags)->values, DATA_REC(rec, btags)->size, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);

  MPI_Send(buf, offset, MPI_PACKED, dest.node, dest.dest, MPI_COMM_WORLD);
  SNetMemFree(buf);
}

void *SNetOutputManager(void *args)
{
  snet_dest_t dest;
  snet_record_t *rec;
  snet_stream_desc_t *sd;

  while (running) {
    sd = SNetStreamPoll(&outgoing);
    rec = (snet_record_t*) SNetStreamGet(sd);
    dest = SNetStreamDestMapGet(streamMap, sd);
    SendRecord(dest, rec);
  }

  return NULL;
}
