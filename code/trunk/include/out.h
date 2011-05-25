#ifndef SNETOUT_HEADER
#define SNETOUT_HEADER

#include <stdarg.h>
#include "snettypes.h"

struct handle *SNetOut(struct handle *hnd, struct record *rec);

/* Arrays of fields, tags and btags are consumed! */
struct handle *SNetOutRawArray(struct handle *hnd, int if_id,
                               snet_variant_t *variant, snet_ref_t **fields,
                               int *tags, int *btags);

struct handle *SNetOutRawV( struct handle *hnd, 
                            int id, 
			    int variant_num, 
			    va_list args);
#endif
