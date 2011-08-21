#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#include "dest.h"
#include "distribution.h"
#include "info.h"
#include "iomanagers.h"
#include "memfun.h"
#include "snetentities.h"
#include "threading.h"

int node_location;
snet_info_tag_t prevDest;
snet_info_tag_t infoCounter;

bool debugWait = false;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  int level, *counter;
  snet_dest_t *dest;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level);
  if (level < MPI_THREAD_MULTIPLE) {
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
  dest->dynamicIndex = 0;
  dest->dynamicLoc = 0;

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
      "output_manager", &SNetOutputManager, NULL));

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
      dest->node = loc;
      SNetDistribNewOut(SNetDestCopy(dest), input);

      input = NULL;
    } else if (loc == node_location) {
      if (input == NULL) input = SNetStreamCreate(0);

      SNetDistribNewIn(SNetDestCopy(dest), input);
      dest->node = loc;
    } else {
      dest->node = loc;
    }
  }

  return input;
}

void SNetDistribNewDynamicCon(snet_dest_t *dest)
{
    snet_dest_t *dst = SNetDestCopy(dest);
    snet_startup_fun_t fun = SNetIdToNet(dest->parent);

    snet_info_t *info = SNetInfoInit();
    SNetInfoSetTag(info, prevDest, (uintptr_t) dst,
                  (void* (*)(void*)) &SNetDestCopy);

    SNetLocvecSet(info, SNetLocvecCreate());

    SNetRouteDynamicEnter(info, dest->dynamicIndex, dest->dynamicLoc, NULL);
    SNetRouteUpdate(info, fun(NULL, info, dest->dynamicLoc), dest->parentNode);
    SNetRouteDynamicExit(info, dest->dynamicIndex, dest->dynamicLoc, NULL);

    SNetInfoDestroy(info);
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  dest->dynamicIndex = dynamicIndex;

  int *counter = SNetMemAlloc(sizeof(int));
  *counter = 0;
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);

  if (fun != NULL) {
    dest->parent = SNetNetToId(fun);
    dest->parentNode = node_location;
    dest->dynamicLoc = dynamicLoc;
  }
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun)
{
  int *counter = (int*) SNetInfoDelTag(info, infoCounter);
  SNetMemFree(counter);
  SNetInfoSetTag(info, infoCounter, (uintptr_t) NULL, NULL);
  return;
}
