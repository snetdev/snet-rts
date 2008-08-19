#ifndef BOX_HEADER
#define BOX_HEADER

#include "buffer.h"
#include "handle.h"
#include "typeencode.h"
snet_tl_stream_t *SNetBox(snet_tl_stream_t *instream,
                       void (*boxfun) (snet_handle_t*),
                       snet_box_sign_t *sign);

#endif
