#ifndef _SAC4SNetc_h_
#define _SAC4SNetc_h_

#include <string.h>
#include "typeencode.h"

char *SAC4SNetGenBoxWrapper( char *box_name,
                             snet_input_type_enc_t *type,
                             void *meta_data);

#endif /* _SAC4SNetc_h_*/
