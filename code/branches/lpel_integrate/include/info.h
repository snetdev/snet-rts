#ifndef _SNET_INFO_H_
#define _SNET_INFO_H_

#include "bool.h"

typedef struct snet_info snet_info_t;

#ifdef DISTRIBUTED_SNET
struct snet_routing_context;
#endif /* DISTRIBUTED_SNET */


snet_info_t *SNetInfoInit();
void SNetInfoDestroy(snet_info_t *info);

#ifdef DISTRIBUTED_SNET
void SNetInfoSetRoutingContext(snet_info_t *info, struct snet_routing_context *context);
struct snet_routing_context *SNetInfoGetRoutingContext(snet_info_t *info);

#endif /* DISTRIBUTED_SNET */
#endif /* _SNET_INFO_H_H */
