#ifndef _METADATA_H_
#define _METADATA_H_

#define SNET_MAX_KEYS_PER_COMPONENT 64

typedef struct snet_metadata {
  int num_keys;
  char *keys[SNET_MAX_KEYS_PER_COMPONENT];
  char *values[SNET_MAX_KEYS_PER_COMPONENT];  
}snet_meta_data_enc_t;

const char *SNetMetadataGet(snet_meta_data_enc_t *md, const char *key);

#endif /* _METADATA_H_ */
