#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#include "dest.h"
#include "distribmap.h"
#include "distribution.h"
#include "info.h"
#include "iomanager.h"
#include "memfun.h"
#include "snetentities.h"
#include "threading.h"

int node_location;
snet_info_tag_t prevDest;
snet_info_tag_t infoCounter;

snet_stream_t *stream;
snet_stream_desc_t *sd;
snet_dest_stream_map_t *destMap;

bool debugWait = false;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  int level, *counter;
  snet_dest_t *dest;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level);
  if (level < MPI_THREAD_MULTIPLE) {
    //FIXME: error
    MPI_Abort(MPI_COMM_WORLD, 2);
  }
  MPI_Comm_rank(MPI_COMM_WORLD, &node_location);

  counter = SNetMemAlloc(sizeof(int));
  *counter = 0;

  dest = SNetMemAlloc(sizeof(snet_dest_t));
  dest->node = 0;
  dest->dest = *counter;
  dest->parent = 1;
  dest->parentNode = 0;
  dest->parentIndex = 0;
  dest->blaat = 0;

  destMap = SNetDestStreamMapCreate(0);
  stream = SNetStreamCreate(0);
  sd = SNetStreamOpen(stream, 'w');

  prevDest = SNetInfoCreateTag();
  SNetInfoSetTag(info, prevDest, (uintptr_t) dest,
                 (void* (*)(void*)) &SNetDestCopy);
  infoCounter = SNetInfoCreateTag();
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);

  {
    volatile int i = 0;
    if (debugWait) {
      printf("PID %d (rank #%d) ready for attach\n", getpid(), node_location);
      fflush(stdout);
      while (0 == i) sleep(5);
    }
  }
}

void SNetDistribStart()
{
  SNetThreadingSpawn(
    SNetEntityCreate( ENTITY_other, -1, NULL,
      "output_manager", &SNetOutputManager, stream));

  SNetThreadingSpawn(
    SNetEntityCreate( ENTITY_other, -1, NULL,
      "input_manager", &SNetInputManager, NULL));
}

void SNetDistribStop()
{
  int i, size;
  if (node_location != 0) return;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  for (i = 0; i < size; i++) {
    MPI_Send(&i, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
}

void SNetDistribDestroy()
{
  MPI_Finalize();
}

int SNetDistribGetNodeId(void)
{
  return node_location;
}

bool SNetDistribIsNodeLocation(int location)
{
  return node_location == location;
}

bool SNetDistribIsRootNode(void)
{
  return node_location == 0;
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
  int *counter = (int*) SNetInfoGetTag(info, infoCounter);
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);

  if (dest->node != loc) {
    dest->dest = (*counter)++;

    if (dest->node == node_location) {
      snet_dest_stream_tuple_t *tuple = SNetMemAlloc(sizeof(snet_dest_stream_tuple_t));
      dest->node = loc;
      tuple->dest = SNetDestCopy(dest);
      tuple->stream = input;
      SNetStreamWrite(sd, tuple);

      input = NULL;
    } else if (loc == node_location) {
      if (input == NULL) {
        input = SNetStreamCreate(0);
      }

      dest->node = loc;
      SNetDestStreamMapSet(destMap, SNetDestCopy(dest),
                           SNetStreamOpen(input, 'w'));
    } else {
      dest->node = loc;
    }
  }

  return input;
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  dest->parentIndex = dynamicIndex;

  int *counter = SNetMemAlloc(sizeof(int));
  *counter = 0;
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);

  if (fun != NULL) {
    dest->parent = SNetNetToId(fun);
    dest->parentNode = node_location;
    dest->blaat = dynamicLoc;
  }
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun)
{
  int *counter = (int*) SNetInfoGetTag(info, infoCounter);
  SNetMemFree(counter);
  return;
}
