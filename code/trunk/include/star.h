#ifndef STAR_HEADER
#define STAR_HEADER

#include "snettypes.h"
#include "stream_layer.h"
#include "typeencode.h"
#include "expression.h"

extern snet_tl_stream_t *SNetStar( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				   snet_dist_info_t *info, 
				   int location,
#endif /* DISTRIBUTED_SNET */
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
				   snet_startup_fun_t box_a,
				   snet_startup_fun_t box_b);


extern
snet_tl_stream_t *SNetStarIncarnate( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				     snet_dist_info_t *info, 
				     int location,
#endif /* DISTRIBUTED_SNET */
				     snet_typeencoding_t *type,
				     snet_expr_list_t *guards,
				     snet_startup_fun_t box_a,
				     snet_startup_fun_t box_b);


extern snet_tl_stream_t *SNetStarDet( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				      snet_dist_info_t *info, 
				      int location,
#endif /* DISTRIBUTED_SNET */
                                      snet_typeencoding_t *type,
                                      snet_expr_list_t *guards,
				      snet_startup_fun_t box_a,
				      snet_startup_fun_t box_b);
extern
snet_tl_stream_t *SNetStarDetIncarnate( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
					snet_dist_info_t *info, 
					int location,
#endif /* DISTRIBUTED_SNET */
					snet_typeencoding_t *type,
					snet_expr_list_t *guards,
					snet_startup_fun_t box_a,
					snet_startup_fun_t box_b);

#endif
