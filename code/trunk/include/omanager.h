#ifndef _SNET_OMANAGER_H_
#define _SNET_OMANAGER_H_

#include "stream_layer.h"

void OManagerUpdateRoutingTable(snet_tl_stream_t *stream, int node, int index);

void OManagerCreate();

#endif /* _SNET_OMANAGER_H_ */
