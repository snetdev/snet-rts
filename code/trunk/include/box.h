#ifndef BOX_HEADER
#define BOX_HEADER

#include "buffer.h"
#include "handle.h"
#include "typeencode.h"
snet_buffer_t *SNetBox(snet_buffer_t *inbuf, 
                       void (*boxfun) (snet_handle_t*),
                       snet_box_sign_t *sign);

#endif
