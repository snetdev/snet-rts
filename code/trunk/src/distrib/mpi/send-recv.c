#include <mpi.h>
#include <stdio.h>

#include "bool.h"
#include "dest.h"
#include "memfun.h"
#include "iomanagers.h"
#include "snetentities.h"
#include "threading.h"

static int sendoffset;
static size_t sendsize = 1000;
static char sendbuf[1000];
static int recvoffset;
static size_t recvsize = 1000;
static char recvbuf[1000];

extern int node_location;
extern snet_info_tag_t prevDest;

static void MPIUnpackWrapper(int foo, int *bar)
{
  MPI_Unpack(recvbuf, recvsize, &recvoffset, bar, foo, MPI_INT, MPI_COMM_WORLD);
}

snet_record_t *RecvRecord(snet_dest_t **tmp)
{
  MPI_Status status;
  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  snet_stream_desc_t *sdc;
  snet_dest_t dest = {status.MPI_SOURCE, 0, status.MPI_TAG, 0, 0, 0};
  snet_record_t *rec;

  MPI_Get_count(&status, MPI_PACKED, &recvoffset);
  if (status.MPI_TAG == 0) {
    MPI_Recv(recvbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    return NULL;
  }
  MPI_Recv(recvbuf, recvoffset, MPI_PACKED, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

  recvoffset = 0;
  MPIUnpackWrapper(1, &dest.dest);
  MPIUnpackWrapper(1, &dest.parentIndex);
  MPIUnpackWrapper(1, &dest.parentNode);
  MPIUnpackWrapper(1, &dest.blaat);
  rec = SNetRecDeserialise(&MPIUnpackWrapper);
  *tmp = SNetDestCopy(&dest);

  return rec;
}

static void MPIPackWrapper(int foo, int *bar)
{
  MPI_Pack(bar, foo, MPI_INT, sendbuf, sendsize, &sendoffset, MPI_COMM_WORLD);
}

void SendRecord(snet_dest_t *dest, snet_record_t *rec)
{
  sendoffset = 0;
  MPIPackWrapper(1, &dest->dest);
  MPIPackWrapper(1, &dest->parentIndex);
  MPIPackWrapper(1, &dest->parentNode);
  MPIPackWrapper(1, &dest->blaat);
  SNetRecSerialise(rec, &MPIPackWrapper);
  MPI_Send(sendbuf, sendoffset, MPI_PACKED, dest->node, dest->parent, MPI_COMM_WORLD);
}
