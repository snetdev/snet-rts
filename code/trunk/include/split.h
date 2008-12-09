#ifndef SPLIT_HEADER
#define SPLIT_HEADER

#include "snettypes.h"
#include "stream_layer.h"


typedef struct {
  snet_tl_stream_t *stream;
  int num;
} snet_blist_elem_t;


extern snet_tl_stream_t *SNetSplit( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				    snet_dist_info_t *info, 
				    int location,
#endif /* DISTRIBUTED_SNET */
				    snet_startup_fun_t box_a,
				    int ltag, 
				    int utag);

extern 
snet_tl_stream_t *SNetSplitDet( snet_tl_stream_t *inbuf, 
#ifdef DISTRIBUTED_SNET
				snet_dist_info_t *info, 
				int location,
#endif /* DISTRIBUTED_SNET */
				snet_startup_fun_t box_a,
				int ltag,
				int utag);

#ifdef DISTRIBUTED_SNET
extern snet_tl_stream_t *SNetLocSplit(snet_tl_stream_t *instream,
				      snet_dist_info_t *info, 
				      int location,
				      snet_startup_fun_t box_a,
				      int ltag, int utag);

extern 
snet_tl_stream_t *SNetLocSplitDet(snet_tl_stream_t *instream,
				  snet_dist_info_t *info, 
				  int location,
				  snet_startup_fun_t box_a,
				  int ltag,
				  int utag);
#endif /* DISTRIBUTED_SNET */
#endif
