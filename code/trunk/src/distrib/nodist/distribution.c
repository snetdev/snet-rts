#include "distribution.h"

static int node_location;

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

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int location)
{
    return input;
}

snet_stream_t *SNetRouteUpdateDynamic(snet_info_t *info, snet_stream_t *input, int location, snet_startup_fun_t start)
{
  return input;
}
