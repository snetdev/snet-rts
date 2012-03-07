#ifndef _C4SNetc_h_
#define _C4SNetc_h_

#include <string.h>
#include "metadata.h"
#include "type.h"

char *C4SNetGenBoxWrapper( char *box_name,
                           snetc_type_enc_t *type,
                           int num_out_types,
                           snetc_type_enc_t **out_types,
                           snet_meta_data_enc_t *meta_data);
#endif /* _C4SNetc_h_*/
