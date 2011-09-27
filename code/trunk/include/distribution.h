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

/* Provided by common distribution interface in src/distrib/common */
void SNetDistribInit(int argc, char** argv, snet_info_t *info);
void SNetDistribStart(void);
void SNetDistribStop(void);
void SNetDistribWaitExit(void);

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc);
void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun);
void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun);

snet_ref_t *SNetRefCreate(void *data, int interface);
snet_ref_t *SNetRefCopy(snet_ref_t *ref);
void *SNetRefGetData(snet_ref_t *ref);
void *SNetRefTakeData(snet_ref_t *ref);
void SNetRefDestroy(snet_ref_t *ref);


/* Implementation specific functions in src/distrib/implementation */
void SNetDistribGlobalStop(void);

int SNetDistribGetNodeId(void);
bool SNetDistribIsNodeLocation(int location);
bool SNetDistribIsRootNode(void);

void SNetDistribPack(void *src, ...);
void SNetDistribUnpack(void *dst, ...);
#endif /* _SNET_DISTRIBUTION_H_ */
