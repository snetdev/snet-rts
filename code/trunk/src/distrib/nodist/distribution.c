#include <pthread.h>

#include "distribution.h"

int SNetNodeLocation;

void SNetDistribInit(int argc, char **argv) {
    SNetNodeLocation = 0;
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

void SNetDebugTimeGetTime(snet_time_t *time) {
    *time = 0;
}

long SNetDebugTimeGetMilliseconds(snet_time_t *time) {
    return 0;
}

long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *tima_a, snet_time_t *time_b) {
    return 0;
}
