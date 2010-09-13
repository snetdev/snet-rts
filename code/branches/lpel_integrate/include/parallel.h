#ifndef PARALLEL_HEADER
#define PARALLEL_HEADER

#include "snettypes.h"
#include "buffer.h"
#include "stream.h"
#include "typeencode.h"

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct { 
  bool is_match;
  int match_count;
} match_count_t;


stream_t *SNetParallel(stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_list_t *types,
    ...);

stream_t *SNetParallelDet(stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_list_t *types,
    ...);

#endif
