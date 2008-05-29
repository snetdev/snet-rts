#ifndef SYNCRO_HEADER
#define SYNCRO_HEADER
#include "expression.h"
#include "typeencode.h"
#include "buffer.h"


snet_buffer_t *SNetSync( snet_buffer_t *input, 
                         snet_typeencoding_t *outtype,
                         snet_typeencoding_t *patterns,
                         snet_expr_list_t *guards);
#endif
