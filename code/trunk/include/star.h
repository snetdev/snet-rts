#ifndef STAR_HEADER
#define STAR_HEADER

#include "stream_layer.h"
#include "typeencode.h"
#include "expression.h"

extern snet_tl_stream_t *SNetStar( snet_tl_stream_t *inbuf,
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
                                  snet_tl_stream_t*(*box_a)(snet_tl_stream_t*),
                                 snet_tl_stream_t*(*box_b)(snet_tl_stream_t*));


extern
snet_tl_stream_t *SNetStarIncarnate( snet_tl_stream_t *inbuf,
                                          snet_typeencoding_t *type,
                                          snet_expr_list_t *guards,
                                  snet_tl_stream_t*(*box_a)(snet_tl_stream_t*),
                                snet_tl_stream_t *(*box_b)(snet_tl_stream_t*));


extern snet_tl_stream_t *SNetStarDet( snet_tl_stream_t *inbuf,
                                      snet_typeencoding_t *type,
                                      snet_expr_list_t *guards,
                                 snet_tl_stream_t *(*box_a)(snet_tl_stream_t*),
                                snet_tl_stream_t *(*box_b)(snet_tl_stream_t*));
extern
snet_tl_stream_t *SNetStarDetIncarnate( snet_tl_stream_t *inbuf,
                               snet_typeencoding_t *type,
                                  snet_expr_list_t *guards,
                                 snet_tl_stream_t *(*box_a)(snet_tl_stream_t*),
                                snet_tl_stream_t *(*box_b)(snet_tl_stream_t*));


#endif
