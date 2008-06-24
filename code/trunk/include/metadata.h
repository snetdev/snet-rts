#ifndef _METADATA_H_
#define _METADATA_H_

typedef struct metadata{
   int num_entries;
   char **keys;
   char **values;
}snet_meta_data_enc_t;

const char *SNetMetadataGet(snet_meta_data_enc_t *md, const char *key);

#endif /* _METADATA_H_ */
