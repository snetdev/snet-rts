#ifndef SYNCRO_HEADER
#define SYNCRO_HEADER
#include "expression.h"
#include "typeencode.h"
#include "buffer.h"

typedef struct {
  bool is_match;
  int match_count;
} match_count_t;

typedef struct {
  snet_buffer_t *from_buf;
  snet_buffer_t *to_buf;
  char *desc;
} dispatch_info_t;

typedef struct {
  snet_buffer_t *buf;
  int num;
} snet_blist_elem_t;

snet_buffer_t *SNetSync( snet_buffer_t *input, 
                         snet_typeencoding_t *outtype,
                         snet_typeencoding_t *patterns,
                         snet_expr_list_t *guards);
#endif
