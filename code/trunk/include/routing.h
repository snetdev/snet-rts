#ifndef _SNET_ROUTING_H_
#define _SNET_ROUTING_H_

#include "stream_layer.h"
#include "fun.h"

#define SNET_LOCATION_NONE    -1
#define SNET_LOCATION_UNKNOWN -2

typedef struct snet_routing_info snet_routing_info_t;

void SNetRoutingInit(snet_tl_stream_t *omngr);

void SNetRoutingDestroy();

int SNetRoutingGetNewID();


snet_routing_info_t *SNetRoutingInfoInit(int id, int starter, int parent, snet_fun_id_t *fun_id, int tag);

void SNetRoutingInfoDestroy(snet_routing_info_t *info);


snet_tl_stream_t *SNetRoutingInfoUpdate(snet_routing_info_t *info, int node, snet_tl_stream_t *stream);

void SNetRoutingInfoPushLevel(snet_routing_info_t *info);

snet_tl_stream_t *SNetRoutingInfoPopLevel(snet_routing_info_t *info, snet_tl_stream_t *stream);

snet_tl_stream_t *SNetRoutingInfoFinalize(snet_routing_info_t *info, snet_tl_stream_t *stream);



snet_tl_stream_t *SNetRoutingGetGlobalInput();

snet_tl_stream_t *SNetRoutingGetGlobalOutput();

snet_tl_stream_t *SNetRoutingWaitForGlobalInput();

snet_tl_stream_t *SNetRoutingWaitForGlobalOutput(); 

void SNetRoutingNotifyAll();

#endif /* SNET_ROUTING_H_ */
