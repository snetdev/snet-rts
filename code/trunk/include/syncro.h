#ifndef SYNCRO_HEADER
#define SYNCRO_HEADER

#include "snettypes.h"
#include "expression.h"
#include "typeencode.h"
#include "stream_layer.h"

snet_tl_stream_t *SNetSync( snet_tl_stream_t *input, 
#ifdef DISTRIBUTED_SNET
			    snet_dist_info_t *info, 
			    int location,
#endif /* DISTRIBUTED_SNET */
			    snet_typeencoding_t *outtype,
			    snet_typeencoding_t *patterns,
			    snet_expr_list_t *guards);
#endif
