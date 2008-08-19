#ifndef SPLIT_HEADER
#define SPLIT_HEADER

#include "stream_layer.h"


typedef struct {
  snet_tl_stream_t *stream;
  int num;
} snet_blist_elem_t;


extern snet_tl_stream_t *SNetSplit( snet_tl_stream_t *inbuf,
                                snet_tl_stream_t* (*box_a)( snet_tl_stream_t*),
                                 int ltag, int utag);

extern 
snet_tl_stream_t *SNetSplitDet( snet_tl_stream_t *inbuf, 
                             snet_tl_stream_t *(*box_a)( snet_tl_stream_t*),
                                       int ltag,
                                       int utag);
#endif
