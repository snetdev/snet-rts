#include <mpi.h>
#include <string.h>

#include "dest.h"
#include "memfun.h"
#include "iomanagers.h"
#include "record.h"

static int offset = 0;
static size_t size = 0;
static char *buf = NULL;

static void MPIPackWrapper(int count, int *src)
{
  int requiredSize;

  MPI_Pack_size(count, MPI_INT, MPI_COMM_WORLD, &requiredSize);
  if (requiredSize > size - offset) {
    char *newBuf = SNetMemAlloc(offset + requiredSize);
    memcpy(newBuf, buf, offset);
    SNetMemFree(buf);
    buf = newBuf;
    size = offset + requiredSize;
  }
  MPI_Pack(src, count, MPI_INT, buf, size, &offset, MPI_COMM_WORLD);
}

void SendRecord(snet_dest_t *dest, snet_record_t *rec)
{
  offset = 0;
  MPIPackWrapper(1, &dest->dest);
  MPIPackWrapper(1, &dest->parent);
  MPIPackWrapper(1, &dest->dynamicIndex);
  MPIPackWrapper(1, &dest->parentNode);
  MPIPackWrapper(1, &dest->dynamicLoc);
  SNetRecSerialise(rec, &MPIPackWrapper);
  MPI_Send(buf, offset, MPI_PACKED, dest->node, 0, MPI_COMM_WORLD);
}
