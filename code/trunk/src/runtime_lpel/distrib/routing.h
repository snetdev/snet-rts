#ifndef _SNET_ROUTING_H_
#define _SNET_ROUTING_H_

#include "bool.h"
#include "snettypes.h"
#include "fun.h"
#include "id.h"

#define SNET_LOCATION_NONE -1

typedef struct snet_routing_context snet_routing_context_t;

void SNetRoutingInit();

void SNetRoutingDestroy();

snet_id_t SNetRoutingGetNewID();

snet_routing_context_t *SNetRoutingContextInit(snet_id_t id, bool is_master, int parent, snet_fun_id_t *fun_id, int tag);

snet_routing_context_t *SNetRoutingContextCopy(snet_routing_context_t *original);

void SNetRoutingContextDestroy(snet_routing_context_t *context);

void SNetRoutingContextSetLocation(snet_routing_context_t *context, int location);
int SNetRoutingContextGetLocation(snet_routing_context_t *context);

void SNetRoutingContextSetParent(snet_routing_context_t *context, int parent);
int SNetRoutingContextGetParent(snet_routing_context_t *context);

snet_stream_t *SNetRoutingContextUpdate(snet_routing_context_t *context, snet_stream_t* stream, int location);

snet_stream_t *SNetRoutingContextEnd(snet_routing_context_t *context, snet_stream_t* stream);

snet_stream_t *SNetRoutingGetGlobalInput();

snet_stream_t *SNetRoutingGetGlobalOutput();

snet_stream_t *SNetRoutingWaitForGlobalInput();

snet_stream_t *SNetRoutingWaitForGlobalOutput(); 

void SNetRoutingNotifyAll();

#endif /* SNET_ROUTING_H_ */
