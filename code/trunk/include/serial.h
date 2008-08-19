#ifndef SERIAL_HEADER
#define SERIAL_HEADER
#include "stream_layer.h"
snet_tl_stream_t *SNetSerial(snet_tl_stream_t *input,
                            snet_tl_stream_t *(*box_a)(snet_tl_stream_t*),
                            snet_tl_stream_t *(*box_b)(snet_tl_stream_t*));
#endif
