#ifndef STAR_HEADER
#define STAR_HEADER

#include "snettypes.h"
#include "stream.h"
#include "typeencode.h"
#include "expression.h"

extern stream_t *SNetStar( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


extern stream_t *SNetStarIncarnate( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


extern stream_t *SNetStarDet( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);
extern
stream_t *SNetStarDetIncarnate( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

#endif
