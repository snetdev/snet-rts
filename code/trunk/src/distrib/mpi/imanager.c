#include <mpi.h>
#include "memfun.h"
#include "distribmap.h"

extern snet_dest_stream_map_t *destMap;
extern int node_location;

static bool running = true;

void RecvRecord(MPI_Status status)
{
  snet_stream_desc_t *sd;
  snet_dest_t dest = {node_location, status.MPI_TAG, 0};
  snet_record_t *rec = SNetRecCreate(REC_data);
  int tmp[2];
  int count;
  size_t size = 1000;
  char *buf = SNetMemAlloc(size);

  MPI_Get_count(&status, MPI_PACKED, &count);
  MPI_Recv(buf, count, MPI_PACKED, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

  count = 0;
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, tags)->size, 1, MPI_INT, MPI_COMM_WORLD);
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, tags)->used, 1, MPI_INT, MPI_COMM_WORLD);
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, btags)->size, 1, MPI_INT, MPI_COMM_WORLD);
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, btags)->used, 1, MPI_INT, MPI_COMM_WORLD);
  MPI_Unpack(buf, size, &count, tmp, 1, MPI_INT, MPI_COMM_WORLD);
  DATA_REC(rec, mode) = tmp[0];

  DATA_REC(rec, tags)->keys = SNetMemAlloc(DATA_REC(rec, tags)->size * sizeof(int));
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, tags)->keys, DATA_REC(rec, tags)->size, MPI_INT, MPI_COMM_WORLD);

  DATA_REC(rec, tags)->values = SNetMemAlloc(DATA_REC(rec, tags)->size * sizeof(int));
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, tags)->values, DATA_REC(rec, tags)->size, MPI_INT, MPI_COMM_WORLD);

  DATA_REC(rec, btags)->keys = SNetMemAlloc(DATA_REC(rec, tags)->size * sizeof(int));
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, btags)->keys, DATA_REC(rec, btags)->size, MPI_INT, MPI_COMM_WORLD);

  DATA_REC(rec, btags)->values = SNetMemAlloc(DATA_REC(rec, tags)->size * sizeof(int));
  MPI_Unpack(buf, size, &count, &DATA_REC(rec, btags)->values, DATA_REC(rec, btags)->size, MPI_INT, MPI_COMM_WORLD);

  sd = SNetDestStreamMapGet(destMap, dest);
  SNetStreamWrite(sd, rec);
  SNetMemFree(buf);
}

void *SNetInputManager(void *args)
{
  MPI_Status status;

  while (running) {
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
      default:
        /*assert(0); //Unknown message*/
        RecvRecord(status);
        break;
    }
  }
  return NULL;
}
