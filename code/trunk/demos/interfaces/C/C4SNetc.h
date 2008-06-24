#ifndef _C4SNetc_h_
#define _C4SNetc_h_

#include <string.h>
#include "typeencode.h"
#include "metadata.h"

char *C4SNetGenBoxWrapper( char *box_name,
                           snet_input_type_enc_t *type,
                           snet_meta_data_enc_t *meta_data);

#endif /* _C4SNetc_h_*/
