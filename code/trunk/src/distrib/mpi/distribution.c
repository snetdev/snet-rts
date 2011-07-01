#include <assert.h>
#include <mpi.h>
#include <pthread.h>

#include "distribution.h"
#include "threading.h"
#include "memfun.h"
#include "info.h"
#include "distribmap.h"

extern void *SNetOutputManager(void *);
extern void *SNetInputManager(void *);

snet_stream_dest_map_t *streamMap;
snet_dest_stream_map_t *destMap;

int node_location;
static snet_info_tag_t prevDest;

bool SNetDestCompare(snet_dest_t d1, snet_dest_t d2)
{
  return d1.node == d2.node && d1.dest == d2.dest && d1.dynamicParent == d2.dynamicParent;
}

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  int level;
  snet_dest_t *dest = SNetMemAlloc(sizeof(snet_dest_t));

  dest->node = 0;
  dest->dest = 0;
  dest->dynamicParent = 0;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level);
  if (level < MPI_THREAD_MULTIPLE) {
    //FIXME: error
    MPI_Abort(MPI_COMM_WORLD, 2);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &node_location);

  prevDest = SNetInfoCreateTag();

  SNetInfoSetTag(info, prevDest, (uintptr_t) dest);
}

void SNetDistribStart()
{
  int err;
  pthread_t p;

  err = pthread_create(&p, NULL, &SNetOutputManager, NULL);
  assert(err == 0);

  err = pthread_create(&p, NULL, &SNetInputManager, NULL);
  assert(err == 0);
}

void SNetDistribStop()
{
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

extern snet_streamset_t outgoing;

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
  snet_stream_desc_t *sd;
  snet_dest_t *dest = (snet_dest_t*) SNetInfoGetTag(info, prevDest);
  dest->dest++;

  if (dest->node != loc) {
    dest->node = loc;

    if (dest->node == node_location) {
      sd = SNetStreamOpen(input, 'r');
      SNetStreamDestMapSet(streamMap, sd, *dest);

      SNetStreamsetPut(&outgoing, sd);
      input = SNetStreamCreate(0);
    } else if (loc == node_location) {
      sd = SNetStreamOpen(input, 'w');
      SNetDestStreamMapSet(destMap, *dest, sd);
    }

  }

  return input;
}

snet_stream_t *SNetRouteUpdateDynamic(snet_info_t *info, snet_stream_t *input,
                                      int loc, snet_startup_fun_t fun)
{
  /*
  snet_stream_desc_t *sd;
  snet_dest_t *dest = SNetInfoGetTag(info, prevDest);
  dest->dest++;

  if (dest->node != loc) {
    if (dest->node == node_location) {
      sd = SNetStreamOpen(input, 'r');
      SNetStreamDestMapSet(streamMap, sd, dest);

      SNetStreamsetPut(outgoing, sd);
      input = SNetStreamCreate(0);
    } else if (loc == node_location) {
      sd = SNetStreamOpen(input, 'w');
      SNetDestStreamMapSet(destMap, dest, sd);
    }

    dest->node = loc;
  }
*/
  return input;
}
