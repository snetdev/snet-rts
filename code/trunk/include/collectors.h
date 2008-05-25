#ifndef COLLECTORS_HEADERS
#define COLLECTORS_HEADERS

#include "buffer.h"
#include "record.h"

typedef struct {
  snet_buffer_t *from_buf;
  snet_buffer_t *to_buf;             
  char *desc;
} dispatch_info_t;

typedef struct {
  snet_buffer_t *buf;
  snet_record_t *current;
} snet_collect_elem_t;

snet_buffer_t *CreateCollector(snet_buffer_t *initial_buffer);
snet_buffer_t *CreateDetCollector(snet_buffer_t *initial_buffer);
#endif
