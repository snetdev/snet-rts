#ifndef SNETOUT_HEADER
#define SNETOUT_HEADER
#include "handle.h"
#include "record.h"

snet_handle_t *SNetOut(snet_handle_t *hnd, snet_record_t *rec);
snet_handle_t *SNetOutRawArray(snet_handle_t *hnd, int if_id, int variant_num,
                                void **fields, int *tags, int *btags);
snet_handle_t *SNetOutRaw(snet_handle_t *hnd, int variant_num, ...);
#endif
