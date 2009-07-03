#ifndef _SNET_ROUTING_H_
#define _SNET_ROUTING_H_

#include "bool.h"
#include "stream_layer.h"
#include "fun.h"

#define SNET_LOCATION_NONE -1

typedef struct snet_routing_context snet_routing_context_t;

void SNetRoutingInit();

void SNetRoutingDestroy();

int SNetRoutingGetNewID();

snet_routing_context_t *SNetRoutingContextInit(int id, bool is_master, int parent, snet_fun_id_t *fun_id, int tag);

void SNetRoutingContextDestroy(snet_routing_context_t *context);

int SNetRoutingContextGetID(snet_routing_context_t *context);

void SNetRoutingContextSetLocation(snet_routing_context_t *context, int location);
int SNetRoutingContextGetLocation(snet_routing_context_t *context);

void SNetRoutingContextSetParent(snet_routing_context_t *context, int parent);
int SNetRoutingContextGetParent(snet_routing_context_t *context);

bool SNetRoutingContextIsMaster(snet_routing_context_t *context);

snet_fun_id_t *SNetRoutingContextGetFunID(snet_routing_context_t *context);

void SNetRoutingContextSetNodeVisited(snet_routing_context_t *context, int node);
bool SNetRoutingContextIsNodeVisited(snet_routing_context_t *context, int node);

int SNetRoutingContextGetTag(snet_routing_context_t *context);

snet_tl_stream_t *SNetRoutingContextUpdate(snet_routing_context_t *context, snet_tl_stream_t* stream, int location);

snet_tl_stream_t *SNetRoutingContextEnd(snet_routing_context_t *context, snet_tl_stream_t* stream);

snet_tl_stream_t *SNetRoutingGetGlobalInput();

snet_tl_stream_t *SNetRoutingGetGlobalOutput();

snet_tl_stream_t *SNetRoutingWaitForGlobalInput();

snet_tl_stream_t *SNetRoutingWaitForGlobalOutput(); 

void SNetRoutingNotifyAll();

#endif /* SNET_ROUTING_H_ */
