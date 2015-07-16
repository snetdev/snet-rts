#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include <stdint.h>
#include <stddef.h>
typedef struct snet_ref snet_ref_t;

#include "info.h"
#include "stream.h"
#include "bool.h"

/* The root node in distributed S-Net. */
#define ROOT_LOCATION           0

/* Provided by common distribution interface in src/distrib/common */
void SNetDistribInit(int argc, char** argv, snet_info_t *info);
void SNetDistribStart(void);
void SNetDistribStop(void);
void SNetDistribWaitExit(snet_info_t *info);

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

/* set call back functions on data */
void SNetReferenceSetDataFunc(size_t (*get_size)(void *));

/* get size of referenced data */
size_t SNetRefGetDataSize(void *data);

/* Implementation specific functions in src/distrib/implementation */
void SNetDistribGlobalStop(void);

int SNetDistribGetNodeId(void);
bool SNetDistribIsNodeLocation(int location);
bool SNetDistribIsRootNode(void);
bool SNetDistribIsDistributed(void);

void SNetDistribPack(void *src, ...);
void SNetDistribUnpack(void *dst, ...);
#endif /* _SNET_DISTRIBUTION_H_ */
