#ifndef SYNCRO_HEADER
#define SYNCRO_HEADER
#include "expression.h"
#include "typeencode.h"
#include "stream_layer.h"

snet_tl_stream_t *SNetSync( snet_tl_stream_t *input, 
                         snet_typeencoding_t *outtype,
                         snet_typeencoding_t *patterns,
                         snet_expr_list_t *guards);
#endif
