#ifndef ALIAS_HEADER
#define ALIAS_HEADER

#include "buffer.h"

snet_buffer_t *SNetAlias( snet_buffer_t *inbuf,
                         snet_buffer_t *(*net)(snet_buffer_t*));

#endif
