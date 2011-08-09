#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "distribution.h"
#include "threading.h"
#include "memfun.h"
#include "info.h"
#include "distribmap.h"

bool debugWait = false;
bool outputDistribInfo = false;
extern snet_stream_dest_map_t *streamMap;

extern void SNetOutputManager(void *);
extern void SNetInputManager(void *);

snet_dest_stream_map_t *destMap;
snet_dynamic_map_t *dynamicMap;

bool globalRunning = true;
int node_location;
int counter = 0;
int parentCounter = 1;
snet_info_tag_t prevDest;
snet_stream_t *stream;
snet_stream_desc_t *sd;

bool SNetDestCompare(snet_dest_t d1, snet_dest_t d2)
{
  return d1.node == d2.node && d1.dest == d2.dest &&
         d1.parent == d2.parent && d1.parentIndex == d2.parentIndex;
}

void *SNetDestCopy(void *d)
{
  snet_dest_t *result = SNetMemAlloc(sizeof(snet_dest_t));
  *result = *(snet_dest_t*)d;
  return result;
}

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  int level;
  snet_dest_t *dest = SNetMemAlloc(sizeof(snet_dest_t));

  dest->node = 0;
  dest->dest = counter;
  dest->parent = parentCounter;
  dest->parentNode = 0;
  dest->parentIndex = 0;

  streamMap = SNetStreamDestMapCreate(0);
  destMap = SNetDestStreamMapCreate(0);
  dynamicMap = SNetDynamicMapCreate(0);

  stream = SNetStreamCreate(0);
  sd = SNetStreamOpen(stream, 'w');

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level);
  if (level < MPI_THREAD_MULTIPLE) {
    //FIXME: error
    MPI_Abort(MPI_COMM_WORLD, 2);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &node_location);

  prevDest = SNetInfoCreateTag();
  SNetInfoSetTag(info, prevDest, (uintptr_t) dest, &SNetDestCopy);

  if (debugWait) {
    int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d (rank #%d) ready for attach\n", getpid(), node_location);
    fflush(stdout);
    while (0 == i)
        sleep(5);
  }
}

void SNetDistribStart()
{

  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_other, -1, NULL,
        "output_manager", &SNetOutputManager, stream)
      );

  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_other, -1, NULL,
        "input_manager", &SNetInputManager, NULL)
      );
}

void SNetDistribStop()
{
  int i, size;
  if (node_location == 0) {
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    for (i = 0; i < size; i++) {
      MPI_Send(&i, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
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

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc, snet_startup_fun_t fun)
{
  int current_node;
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  counter++;
  dest->dest = counter;
  if (fun != NULL) {
    parentCounter++;
    dest->parent = parentCounter;
    SNetDynamicMapSet(dynamicMap, parentCounter, fun);
  }
  current_node = dest->node;

  if (dest->node != loc) {
    if (dest->node == node_location) {
      dest->node = loc;
      SNetStreamDestMapSet(streamMap, input, *dest);
      SNetStreamWrite(sd, SNetRecCreate(REC_collect, input));

      if (outputDistribInfo) {
        fprintf(stderr, "Node #%d: Added outgoing #%d.\n", node_location,
                dest->dest);
      }
      input = NULL;
    } else if (loc == node_location) {
      dest->node = loc;
      if (input == NULL) {
        input = SNetStreamCreate(0);
      }

      if (outputDistribInfo) {
        fprintf(stderr, "Node #%d: Added incoming #%d.\n", node_location,
                dest->dest);
      }
      SNetDestStreamMapSet(destMap, *dest, SNetStreamOpen(input, 'w'));
    } else {
      dest->node = loc;
    }
  }

  if (outputDistribInfo) {
    fprintf(stderr, "SNet (Node #%d):\n  Dest.dest: %d\n  Origin: %d\n  Destination: %d\n  Parent: %d\n  Parent index: %d\n", node_location, dest->dest, current_node, loc, dest->parent, dest->parentIndex);
  }

  return input;
}

void SNetRouteUpdateDynamic(snet_info_t *info, int parentIndex, bool start)
{
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  counter = 0;
  dest->parentIndex = parentIndex;
  if (start) {
    dest->parentNode = node_location;
  }

  if (outputDistribInfo) {
    fprintf(stderr, "SNet (Node #%d) (dynamic):\n  Dest.dest: %d\n  Origin: %d\n  Parent: %d\n  Parent index: %d\n", node_location, dest->dest, dest->node, dest->parent, dest->parentIndex);
  }
}
