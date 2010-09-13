#ifndef SYNCRO_HEADER
#define SYNCRO_HEADER

#include "snettypes.h"
#include "expression.h"
#include "typeencode.h"
#include "stream.h"

extern stream_t *SNetSync( stream_t *input, 
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *outtype,
    snet_typeencoding_t *patterns,
    snet_expr_list_t *guards);
#endif
