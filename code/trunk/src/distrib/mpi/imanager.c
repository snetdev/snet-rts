#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include "info.h"
#include "memfun.h"
#include "entities.h"
#include "distribution.h"
#include "distribmap.h"

static int offset;
static size_t size = 1000;
static char buf[1000];
static bool running = true;
extern int node_location;
extern bool outputDistribInfo;
extern snet_dynamic_map_t *dynamicMap;
extern snet_dest_stream_map_t *destMap;
extern snet_info_tag_t prevDest;
extern snet_stream_desc_t *sd;
extern void *SNetDestCopy(void *d);

static void MPIUnpackWrapper(int foo, int *bar)
{
  MPI_Unpack(buf, size, &offset, bar, foo, MPI_INT, MPI_COMM_WORLD);
}

void RecvRecord(MPI_Status status)
{
  snet_stream_desc_t *sdc;
  snet_dest_t dest = {SNetDistribGetNodeId(), 0, status.MPI_TAG, 0, 0};
  snet_record_t *rec;

  MPI_Get_count(&status, MPI_PACKED, &offset);
  if (status.MPI_TAG == 0) {
    MPI_Recv(buf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    SNetStreamWrite(sd, NULL);
    assert(SNetDestStreamMapSize(destMap) == 0);
    running = false;
    return;
  }
  MPI_Recv(buf, offset, MPI_PACKED, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

  offset = 0;
  MPIUnpackWrapper(1, &dest.dest);
  MPIUnpackWrapper(1, &dest.parentIndex);
  MPIUnpackWrapper(1, &dest.parentNode);
  rec = SNetRecDeserialise(&MPIUnpackWrapper);

  //fprintf(stderr, "#%d: Recieved message for: %d, %d, %d, %d\n", node_location, dest.node, dest.dest, dest.parent, dest.parentIndex);
  //fflush(stderr);
  if (!SNetDestStreamMapContains(destMap, dest)) {
    snet_dest_t *dst = SNetMemAlloc(sizeof(snet_dest_t));
    *dst = dest;
    dst->node = status.MPI_SOURCE;
    snet_startup_fun_t fun = SNetDynamicMapGet(dynamicMap, dest.parent);

    snet_info_t *info = SNetInfoInit();
    SNetInfoSetTag(info, prevDest, (uintptr_t) dst, &SNetDestCopy);
    SNetLocvecSet(info, SNetLocvecCreate());
    snet_stream_t *str = SNetStreamCreate(0);
    SNetDestStreamMapSet(destMap, dest, SNetStreamOpen(str, 'w'));
    SNetRouteUpdateDynamic(info, dest.parentIndex, false);
    str = fun(str, info, node_location);
    str = SNetRouteUpdate(info, str, dest.parentNode, NULL);
    SNetInfoDestroy(info);
  }
  if (REC_DESCR(rec) == REC_terminate) {
    sdc = SNetDestStreamMapTake(destMap, dest);
  } else {
    sdc = SNetDestStreamMapGet(destMap, dest);
  }
  SNetStreamWrite(sdc, rec);
}

void SNetInputManager(snet_entity_t *ent, void *args)
{
  MPI_Status status;

  while (running) {
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    RecvRecord(status);
  }
}
