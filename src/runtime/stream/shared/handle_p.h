#ifndef _HANDLE_P_H_
#define _HANDLE_P_H_

#include "handle.h"
#include "locvec.h"
#include "threading.h"

typedef struct {
  int num;
  char **string_names;
  int *int_names;
} name_mapping_t;


struct handle {
  snet_record_t *rec;
  snet_int_list_list_t *sign;
  name_mapping_t *mapping;
  snet_stream_desc_t *out_sd;
  snet_variant_list_t *vars;
  void *cdata;
};

void SNetHndDestroy( snet_handle_t *hnd);

#endif /* _HANDLE_P_H_ */
