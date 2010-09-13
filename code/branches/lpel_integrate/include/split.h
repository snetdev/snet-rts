#ifndef SPLIT_HEADER
#define SPLIT_HEADER

#include "snettypes.h"
#include "stream.h"


typedef struct {
  stream_mh_t *stream;
  int num;
} snet_blist_elem_t;


extern stream_t *SNetSplit( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, 
    int utag);

extern stream_t *SNetSplitDet( stream_t *inbuf, 
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag,
    int utag);

#ifdef DISTRIBUTED_SNET
extern stream_t *SNetLocSplit( stream_t *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

extern stream_t *SNetLocSplitDet( stream_t *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag);
#endif /* DISTRIBUTED_SNET */
#endif
