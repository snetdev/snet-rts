#ifndef _C4SNetc_h_
#define _C4SNetc_h_

#include <string.h>
#include "typeencode.h"

char *C4SNetGenBoxWrapper( char *box_name,
                           snet_input_type_enc_t *type,
                           void *meta_data);

#endif /* _C4SNetc_h_*/
