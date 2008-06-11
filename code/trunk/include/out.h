#ifndef SNETOUT_HEADER
#define SNETOUT_HEADER

#include <stdarg.h>
#include "handle.h"
#include "record.h"

snet_handle_t *SNetOut(snet_handle_t *hnd, snet_record_t *rec);

/* Arrays of fields, tags and btags are consumed! */
snet_handle_t *SNetOutRawArray(snet_handle_t *hnd, int if_id, int variant_num,
                               void **fields, int *tags, int *btags);


snet_handle_t *SNetOutRaw( snet_handle_t *hnd, 
                            int id, 
			    int variant_num, 
			    ...);

snet_handle_t *SNetOutRawV( snet_handle_t *hnd, 
                            int id, 
			    int variant_num, 
			    va_list args);
#endif
