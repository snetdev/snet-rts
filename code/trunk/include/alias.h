#ifndef ALIAS_HEADER
#define ALIAS_HEADER

#include "stream_layer.h"

snet_tl_stream_t *SNetAlias( snet_tl_stream_t *inbuf,
                         snet_tl_stream_t *(*net)(snet_tl_stream_t*));

#endif
