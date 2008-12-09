#ifndef ALIAS_HEADER
#define ALIAS_HEADER

#include "snettypes.h"
#include "stream_layer.h"

snet_tl_stream_t *SNetAlias(snet_tl_stream_t *instream,
#ifdef DISTRIBUTED_SNET
			    snet_dist_info_t *info, 
			    int location,
#endif /* DISTRIBUTED_SNET */ 
			    snet_startup_fun_t net);

#endif
