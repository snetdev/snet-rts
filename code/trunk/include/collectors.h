#ifndef COLLECTORS_HEADERS
#define COLLECTORS_HEADERS

#include "buffer.h"
#include "record.h"

snet_buffer_t *CreateCollector(snet_buffer_t *initial_buffer);
snet_buffer_t *CreateDetCollector(snet_buffer_t *initial_buffer);
#endif
