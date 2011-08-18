#include <mpi.h>
#include <stdio.h>

#include "bool.h"
#include "dest.h"
#include "memfun.h"
#include "iomanager.h"
#include "snetentities.h"
#include "threading.h"

static int sendoffset;
static size_t sendsize = 1000;
static char sendbuf[1000];
static int recvoffset;
static size_t recvsize = 1000;
static char recvbuf[1000];

extern int node_location;
extern snet_dest_stream_map_t *destMap;
extern snet_info_tag_t prevDest;

static void MPIUnpackWrapper(int foo, int *bar)
{
  MPI_Unpack(recvbuf, recvsize, &recvoffset, bar, foo, MPI_INT, MPI_COMM_WORLD);
}

bool RecvRecord()
{
  MPI_Status status;
  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  snet_stream_desc_t *sdc;
  snet_dest_t dest = {SNetDistribGetNodeId(), 0, status.MPI_TAG, 0, 0, 0};
  snet_record_t *rec;

  MPI_Get_count(&status, MPI_PACKED, &recvoffset);
  if (status.MPI_TAG == 0) {
    MPI_Recv(recvbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    return false;
  }
  MPI_Recv(recvbuf, recvoffset, MPI_PACKED, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

  recvoffset = 0;
  MPIUnpackWrapper(1, &dest.dest);
  MPIUnpackWrapper(1, &dest.parentIndex);
  MPIUnpackWrapper(1, &dest.parentNode);
  MPIUnpackWrapper(1, &dest.blaat);
  rec = SNetRecDeserialise(&MPIUnpackWrapper);

  if (!SNetDestStreamMapContains(destMap, &dest)) {
    snet_dest_t *dst = SNetMemAlloc(sizeof(snet_dest_t));
    *dst = dest;
    dst->node = status.MPI_SOURCE;
    snet_startup_fun_t fun = SNetIdToNet(dest.parent);

    snet_info_t *info = SNetInfoInit();
    SNetInfoSetTag(info, prevDest, (uintptr_t) dst, (void* (*)(void*)) &SNetDestCopy);
    SNetLocvecSet(info, SNetLocvecCreate());
    snet_stream_t *str = SNetStreamCreate(0);
    SNetRouteDynamicEnter(info, dest.parentIndex, dest.blaat, NULL);
    str = fun(str, info, dest.blaat);
    str = SNetRouteUpdate(info, str, dest.parentNode);
    SNetRouteDynamicExit(info, dest.parentIndex, dest.blaat, NULL);
    SNetInfoDestroy(info);
  }
  if (REC_DESCR(rec) == REC_terminate) {
    sdc = SNetDestStreamMapTake(destMap, &dest);
  } else {
    sdc = SNetDestStreamMapGet(destMap, &dest);
  }
  SNetStreamWrite(sdc, rec);

  return true;
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
