#ifndef PARALLEL_HEADER
#define PARALLEL_HEADER

#include "buffer.h"
#include "typeencode.h"

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct { 
  bool is_match;
  int match_count;
} match_count_t;

snet_buffer_t *SNetParallel(snet_buffer_t *inbuf, 
                             snet_typeencoding_list_t *types,
                             ...);

snet_buffer_t *SNetParallelDet(snet_buffer_t *inbuf, 
                                snet_typeencoding_list_t *types,
                                ...);
#endif
