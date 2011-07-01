#ifndef SNETOUT_HEADER
#define SNETOUT_HEADER

#include <stdarg.h>
#include "variant.h"
#include "record.h"
#include "handle.h"

struct handle *SNetOut(struct handle *hnd, struct record *rec);

/* Arrays of fields, tags and btags are consumed! */
struct handle *SNetOutRawArray(struct handle *hnd, int if_id,
                               snet_variant_t *variant, void **fields,
                               int *tags, int *btags);

struct handle *SNetOutRawV( struct handle *hnd, 
                            int id, 
			    int variant_num, 
			    va_list args);
#endif
