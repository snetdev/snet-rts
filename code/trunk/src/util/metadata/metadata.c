#include "metadata.h"
#include <string.h>

const char *SNetMetadataGet(snet_meta_data_enc_t *md, const char *key) {
  int i;

  if(md == NULL || key == NULL) {
    return NULL;
  };

  for(i = 0; i < md->num_entries; i++) {
    if(strcmp(md->keys[i], key) == 0) {
      return md->values[i];
    }
  }
  return NULL;
}
