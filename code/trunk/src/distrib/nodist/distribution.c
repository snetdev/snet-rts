#include "distribution.h"

static int node_location;
bool debugWait, outputDistribInfo;

void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
    node_location = 0;
}

void SNetDistribStart()
{
}

void SNetDistribStop()
{
}

void SNetDistribDestroy()
{
}

int SNetDistribGetNodeId(void)
{
  return node_location;
}

bool SNetDistribIsNodeLocation(int location)
{
  /* with nodist, all entities should be built */
  return true;
}

bool SNetDistribIsRootNode(void)
{
  return true;
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc)
{
    return input;
}

void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  return;
}

void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun)
{
  return;
}
