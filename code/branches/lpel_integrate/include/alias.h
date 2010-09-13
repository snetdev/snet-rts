#ifndef ALIAS_HEADER
#define ALIAS_HEADER

#include "snettypes.h"
#include "stream.h"

stream_t *SNetAlias(stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_startup_fun_t net);

#endif
