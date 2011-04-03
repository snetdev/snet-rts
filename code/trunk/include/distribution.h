#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

typedef void* snet_ref_t;
typedef struct snet_stream_t snet_stream_t;

#include "info.h"

extern int SNetNodeLocation;

void SNetDistribInit(int argc, char** argv);
void SNetDistribStart(snet_info_t *info);
void SNetDistribStop();
void SNetDistribDestroy();

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int location);
#endif /* _SNET_DISTRIBUTION_H_ */
