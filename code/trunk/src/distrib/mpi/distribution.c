#include "distribution.h"

int node_location;

void SNetDistribInit(int argc, char **argv) {
}

void SNetDistribStart(snet_info_t *info) {
}

void SNetDistribStop() {
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

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int location) {
    return input;
}
