#ifndef _SAC4SNetc_h_
#define _SAC4SNetc_h_

#include <string.h>
#include "typeencode.h"
#include "metadata.h"

char *SAC4SNetGenBoxWrapper( char *box_name,
                             snet_input_type_enc_t *type,
                             snet_meta_data_enc_t *meta_data);

#endif /* _SAC4SNetc_h_*/
