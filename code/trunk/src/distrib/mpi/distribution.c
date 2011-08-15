#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "distribution.h"
#include "snetentities.h"
#include "threading.h"
#include "memfun.h"
#include "info.h"
#include "distribmap.h"

bool debugWait = false;
bool outputDistribInfo = false;
extern snet_stream_dest_map_t *streamMap;

typedef struct {
  snet_dest_t dest;
  snet_stream_t *stream;
} tmp_t;

extern void SNetOutputManager(void *);
extern void SNetInputManager(void *);

snet_dest_stream_map_t *destMap;

bool globalRunning = true;
int node_location;
snet_info_tag_t prevDest;
snet_info_tag_t infoCounter;
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
  int i = 0;
  int level;
  snet_dest_t *dest = SNetMemAlloc(sizeof(snet_dest_t));
  int *counter = SNetMemAlloc(sizeof(int));
  *counter = 0;

  dest->node = 0;
  dest->dest = *counter;
  dest->parent = 1;
  dest->parentNode = 0;
  dest->parentIndex = 0;
  dest->blaat = 0;

  streamMap = SNetStreamDestMapCreate(0);
  destMap = SNetDestStreamMapCreate(0);

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
  infoCounter = SNetInfoCreateTag();
  SNetInfoSetTag(info, infoCounter, (uintptr_t) counter, NULL);
}

void SNetDistribStart()
{
  volatile int i = 0;
  if (debugWait) {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d (rank #%d) ready for attach\n", getpid(), node_location);
    fflush(stdout);
    while (0 == i)
        sleep(5);
  }


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

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
  int current_node;
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  int *counter = (int*) SNetInfoGetTag(info, infoCounter);
  *counter = *counter + 1;
  dest->dest = *counter;
  current_node = dest->node;

  if (dest->node != loc) {
    if (dest->node == node_location) {
      tmp_t *blaat = SNetMemAlloc(sizeof(tmp_t));
      dest->node = loc;
      blaat->dest = *dest;
      blaat->stream = input;
      SNetStreamWrite(sd, blaat);

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
    fprintf(stderr, "Node #%d: D: %d ON: %d DN: %d P: %d PI: %d\n", node_location, dest->dest, current_node, loc, dest->parent, dest->parentIndex);
  }

  return input;
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  int *tmp = SNetMemAlloc(sizeof(int));
  *tmp = 0;
  SNetInfoSetTag(info, infoCounter, (uintptr_t) tmp, NULL);
  dest->parentIndex = dynamicIndex;
  if (fun != NULL) {
    dest->parent = SNetNetToId(fun);
    dest->parentNode = node_location;
    dest->blaat = dynamicLoc;
  }

  if (outputDistribInfo) {
    fprintf(stderr, "Dyn #%d: D: %d ON: %d P: %d PI: %d\n", node_location, dest->dest, dest->node, dest->parent, dest->parentIndex);
  }
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun)
{
  return;
}
