#ifndef COLLECTORS_HEADERS
#define COLLECTORS_HEADERS

#include "buffer.h"
#include "record.h"

snet_tl_stream_t *CreateCollector(snet_tl_stream_t *initial_stream);
snet_tl_stream_t *CreateDetCollector(snet_tl_stream_t *initial_stream);
#endif
