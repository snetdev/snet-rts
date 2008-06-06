#ifndef STAR_HEADER
#define STAR_HEADER

#include "buffer.h"
#include "typeencode.h"
#include "expression.h"

extern snet_buffer_t *SNetStar( snet_buffer_t *inbuf,
                                snet_typeencoding_t *type,
                                snet_expr_list_t *guards,
                                snet_buffer_t* (*box_a)(snet_buffer_t*),
                                snet_buffer_t* (*box_b)(snet_buffer_t*));


extern
snet_buffer_t *SNetStarIncarnate( snet_buffer_t *inbuf,
                            snet_typeencoding_t *type,
                               snet_expr_list_t *guards,
                                  snet_buffer_t *(*box_a)(snet_buffer_t*),
                                  snet_buffer_t *(*box_b)(snet_buffer_t*));


extern snet_buffer_t *SNetStarDet( snet_buffer_t *inbuf,
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
                                   snet_buffer_t *(*box_a)(snet_buffer_t*),
                                   snet_buffer_t *(*box_b)(snet_buffer_t*));
extern
snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf,
                               snet_typeencoding_t *type,
                                  snet_expr_list_t *guards,
                                     snet_buffer_t *(*box_a)(snet_buffer_t*),
                                     snet_buffer_t *(*box_b)(snet_buffer_t*));


#endif
