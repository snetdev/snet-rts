#ifndef SERIAL_HEADER
#define SERIAL_HEADER
#include "buffer.h"

snet_buffer_t *SNetSerial(snet_buffer_t *inbuf, 
                            snet_buffer_t *(*box_a)(snet_buffer_t*),
                            snet_buffer_t *(*box_b)(snet_buffer_t*));
#endif
