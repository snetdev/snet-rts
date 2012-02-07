#ifndef _SAC4SNetc_h_
#define _SAC4SNetc_h_

#include <string.h>
#include "type.h"
#include "metadata.h"

char *SAC4SNetGenBoxWrapper( char *box_name,
                             snetc_type_enc_t *type,
                             int num_out_types,
                             snetc_type_enc_t **out_types,
                             snet_meta_data_enc_t *meta_data);

#endif /* _SAC4SNetc_h_*/
