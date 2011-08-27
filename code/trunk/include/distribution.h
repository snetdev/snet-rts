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
void SNetDistribStart();
void SNetDistribStop();
void SNetDistribDestroy();

int SNetDistribGetNodeId(void);
bool SNetDistribIsNodeLocation(int location);
bool SNetDistribIsRootNode(void);

snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *input, int loc);
void SNetRouteDynamicEnter(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                           snet_startup_fun_t fun);
void SNetRouteDynamicExit(snet_info_t *info, int dynamicIndex, int dynamicLoc,
                          snet_startup_fun_t fun);

snet_ref_t *SNetDistribRefCreate(void *data, int interface);
snet_ref_t *SNetDistribRefCopy(snet_ref_t *ref);
void *SNetDistribRefGetData(snet_ref_t *ref);
void *SNetDistribRefTakeData(snet_ref_t *ref);
void SNetDistribRefDestroy(snet_ref_t *ref);
#endif /* _SNET_DISTRIBUTION_H_ */
