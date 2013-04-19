#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "distribution.h"
#include "distribcommon.h"
#include "pack.h"

int node_location;

void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info)
{
  int level;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level);
  if (level < MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "Required threading level not supported!\nDesired level: "
            "%d\nAvailable: %d\n", MPI_THREAD_MULTIPLE, level);
    MPI_Abort(MPI_COMM_WORLD, 2);
  }
  MPI_Comm_rank(MPI_COMM_WORLD, &node_location);

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-debugWait") == 0) {
      volatile int stop = 0;
      printf("PID %d (rank #%d) ready for attach\n", getpid(), node_location);
      fflush(stdout);
      while (0 == stop) sleep(5);
      break;
    }
  }
}

void SNetDistribGlobalStop(void)
{
  int i;

  MPI_Comm_size(MPI_COMM_WORLD, &i);
  for (i--; i >= 0; i--) {
    MPI_Send(&i, 1, MPI_INT, i, snet_stop, MPI_COMM_WORLD);
  }
}

void SNetDistribLocalStop(void) { MPI_Finalize(); }

int SNetDistribGetNodeId(void) { return node_location; }

bool SNetDistribIsNodeLocation(int loc) { return node_location == loc; }

bool SNetDistribIsRootNode(void) { return node_location == 0; }

void SNetDistribPack(void *src, ...)
{
  va_list args;
  mpi_buf_t *buf;
  MPI_Datatype type;
  int count;

  va_start(args, src);
  buf = va_arg(args, mpi_buf_t *);
  type = va_arg(args, MPI_Datatype);
  count = va_arg(args, int);
  va_end(args);

  MPIPack(buf, src, type, count);
}

void SNetDistribUnpack(void *dst, ...)
{
  va_list args;
  mpi_buf_t *buf;
  MPI_Datatype type;
  int count;

  va_start(args, dst);
  buf = va_arg(args, mpi_buf_t *);
  type = va_arg(args, MPI_Datatype);
  count = va_arg(args, int);
  va_end(args);

  MPIUnpack(buf, dst, type, count);
}

void* SNetDistribRefGetData(snet_ref_t *ref)
{
  if (SNetDistribIsNodeLocation(ref->node)) return (void*) ref->data;

  void *result = 0;
  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  if (refInfo->data == NULL) {
    snet_stream_t *str = SNetStreamCreate(1);
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'r');

    if (refInfo->list) {
      SNetStreamListAppendEnd(refInfo->list, str);
    } else {
      refInfo->list = SNetStreamListCreate(1, str);
      SNetDistribFetchRef(ref);
    }

    // refInfo->count gets updated by SNetRefSet instead of here to avoid a
    // race between nodes when a pointer is recycled.
    pthread_mutex_unlock(&remoteRefMutex);
    result = SNetStreamRead(sd);
    SNetStreamClose(sd, true);

    return result;
  }

  SNetDistribUpdateRef(ref, -1);
  if (--refInfo->count == 0) {
    result = refInfo->data;
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  } else {
    result = COPYFUN(ref->interface, refInfo->data);
  }

  pthread_mutex_unlock(&remoteRefMutex);
  return result;
}

void SNetDistribRefSet(snet_ref_t *ref, void *data)
{
  snet_stream_t *str;
  pthread_mutex_lock(&remoteRefMutex);
  snet_refcount_t *refInfo = SNetRefRefcountMapGet(remoteRefMap, *ref);

  LIST_DEQUEUE_EACH(refInfo->list, str) {
    snet_stream_desc_t *sd = SNetStreamOpen(str, 'w');
    SNetStreamWrite(sd, (void*) COPYFUN(ref->interface, data));
    SNetStreamClose(sd, false);
    // Counter is updated here instead of the fetching side to avoid a race
    // across node boundaries.
    refInfo->count--;
  }

  SNetStreamListDestroy(refInfo->list);

  if (refInfo->count > 0) {
    refInfo->data = data;
  } else {
    FREEFUN(ref->interface, data);
    SNetMemFree(SNetRefRefcountMapTake(remoteRefMap, *ref));
  }

  pthread_mutex_unlock(&remoteRefMutex);
}
