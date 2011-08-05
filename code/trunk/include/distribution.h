#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include "info.h"
#include "stream.h"
#include "bool.h"
#include "snettypes.h"

void SNetDistribInit(int argc, char** argv, snet_info_t *info);
void SNetDistribStart();
void SNetDistribStop();
void SNetDistribDestroy();

int SNetDistribGetNodeId(void);
bool SNetDistribIsNodeLocation(int location);
bool SNetDistribIsRootNode(void);

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc, snet_startup_fun_t start);
void SNetRouteUpdateDynamic(snet_info_t *info, int parentIndex, bool start);
#endif /* _SNET_DISTRIBUTION_H_ */
