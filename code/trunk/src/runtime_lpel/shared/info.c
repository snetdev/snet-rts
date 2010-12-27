#include "info.h"
#ifdef DISTRIBUTED_SNET
#include "../distrib/routing.h"
#endif /* DISTRIBUTED_SNET */
#include "memfun.h"

struct snet_info {
#ifdef DISTRIBUTED_SNET
  snet_routing_context_t *routing_context;
#endif /* DISTRIBUTED_SNET */
};

snet_info_t *SNetInfoInit()
{
  snet_info_t *new;

  new = SNetMemAlloc(sizeof(snet_info_t));

#ifdef DISTRIBUTED_SNET
  new->routing_context = NULL;
#endif /* DISTRIBUTED_SNET */

  return new;
}

void SNetInfoDestroy(snet_info_t *info)
{
#ifdef DISTRIBUTED_SNET
  if(info->routing_context != NULL) {
    SNetRoutingContextDestroy(info->routing_context);
  }
#endif /* DISTRIBUTED_SNET */

  SNetMemFree(info);
}

#ifdef DISTRIBUTED_SNET
void SNetInfoSetRoutingContext(snet_info_t *info, snet_routing_context_t *context)
{
  info->routing_context = context;
}

snet_routing_context_t *SNetInfoGetRoutingContext(snet_info_t *info)
{
  return info->routing_context;
}

#endif /* DISTRIBUTED_SNET */
