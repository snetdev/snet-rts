#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include <snettypes.h>
#include "info.h"

typedef double snet_time_t;
typedef void* snet_ref_t;

extern int SNetNodeLocation;

void SNetDistribInit(int argc, char** argv);
void SNetDistribStart(snet_info_t *info);
void SNetDistribStop();
void SNetDistribDestroy();

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int location);

void SNetDebugTimeGetTime(snet_time_t *time);
long SNetDebugTimeGetMilliseconds(snet_time_t *time);
long SNetDebugTimeDifferenceInMilliseconds(snet_time_t *time_a, snet_time_t *time_b);
#endif /* _SNET_DISTRIBUTION_H_ */
