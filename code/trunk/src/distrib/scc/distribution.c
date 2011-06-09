#include "distribution.h"

int SNetNodeLocation;

void SNetDistribInit(int argc, char **argv) {
}

void SNetDistribStart(snet_info_t *info) {
}

void SNetDistribStop() {
}

void SNetDistribDestroy() {
}

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int location) {
    return input;
}
