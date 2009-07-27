#ifndef __PRINTING_H
#define __PRINTING_H

#include "snet-gwrt.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
print_variant(
    const snet_variantencoding_t *venc, const char *labels[]);

extern void 
print_type(const snet_typeencoding_t *tenc, const char *labels[]);

/*----------------------------------------------------------------------------*/

extern void 
print_record(const snet_record_t *rec, const char *labels[]);

#endif // __PRINTING_H

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

