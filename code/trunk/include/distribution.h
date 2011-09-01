#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include <stdint.h>

typedef struct {
  int node, interface;
  uintptr_t data;
} snet_ref_t;

#include "info.h"
#include "stream.h"
#include "bool.h"

void SNetDistribInit(int argc, char** argv, snet_info_t *info);
void SNetDistribStart(void);
void SNetDistribStop(bool global);
void SNetDistribWaitExit(void);

int SNetDistribGetNodeId(void);
bool SNetDistribIsNodeLocation(int location);
bool SNetDistribIsRootNode(void);

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc);
void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun);
void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun);

void SNetDistribPack(void *src, ...);
void SNetDistribUnpack(void *dst, ...);

snet_ref_t *SNetRefCreate(void *data, int interface);
snet_ref_t *SNetRefCopy(snet_ref_t *ref);
void *SNetRefGetData(snet_ref_t *ref);
void *SNetRefTakeData(snet_ref_t *ref);
void SNetRefDestroy(snet_ref_t *ref);
#endif /* _SNET_DISTRIBUTION_H_ */
