#ifndef BOX_HEADER
#define BOX_HEADER

#include "snettypes.h"
#include "buffer.h"
#include "handle.h"
#include "typeencode.h"

extern stream_t *SNetBox( stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */ 
    const char *boxname,
    snet_box_fun_t fun,
    snet_box_sign_t *sign);

#endif
