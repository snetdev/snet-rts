#include <mpi.h>

#include "dest.h"
#include "memfun.h"
#include "iomanagers.h"
#include "record.h"

static int offset = 0;
static size_t size = 0;
static char *buf = NULL;

static void MPIUnpackWrapper(int count, int *dst)
{
  MPI_Unpack(buf, size, &offset, dst, count, MPI_INT, MPI_COMM_WORLD);
}

snet_record_t *RecvRecord(snet_dest_t **recordDest)
{
  int count;
  MPI_Status status;
  snet_record_t *rec;
  snet_dest_t dest = {0, 0, 0, 0, 0, 0};

  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  dest.node = status.MPI_SOURCE;
  dest.parent = status.MPI_TAG;

  MPI_Get_count(&status, MPI_PACKED, &count);

  MPI_Pack_size(count, MPI_PACKED, MPI_COMM_WORLD, &offset);
  if (offset > size) {
    SNetMemFree(buf);
    buf = SNetMemAlloc(offset);
    size = offset;
  }

  if (status.MPI_TAG == 0) {
    MPI_Recv(buf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    return NULL;
  }

  MPI_Recv(buf, offset, MPI_PACKED, MPI_ANY_SOURCE, status.MPI_TAG,
          MPI_COMM_WORLD, &status);

  offset = 0;
  MPIUnpackWrapper(1, &dest.dest);
  MPIUnpackWrapper(1, &dest.dynamicIndex);
  MPIUnpackWrapper(1, &dest.parentNode);
  MPIUnpackWrapper(1, &dest.dynamicLoc);
  rec = SNetRecDeserialise(&MPIUnpackWrapper);
  *recordDest = SNetDestCopy(&dest);

  return rec;
}
