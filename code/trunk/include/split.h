#ifndef SPLIT_HEADER
#define SPLIT_HEADER

#include "buffer.h"


typedef struct {
  snet_buffer_t *buf;
  int num;
} snet_blist_elem_t;


extern snet_buffer_t *SNetSplit( snet_buffer_t *inbuf,
                                 snet_buffer_t* (*box_a)( snet_buffer_t*),
                                 int ltag, int utag);

extern 
snet_buffer_t *SNetSplitDet( snet_buffer_t *inbuf, 
                             snet_buffer_t *(*box_a)( snet_buffer_t*),
                                       int ltag,
                                       int utag);
#endif
