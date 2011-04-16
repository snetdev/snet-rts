#ifndef VARIANT_H
#define VARIANT_H

typedef struct snet_variant_t snet_variant_t;

#include "list.h"

struct snet_variant_t {
  snet_int_list_t *tags, *btags, *fields;
};

#define VARIANT_FOR_EACH_TAG(var, val)      LIST_FOR_EACH(var->tags, val)
#define VARIANT_FOR_EACH_BTAG(var, val)     LIST_FOR_EACH(var->btags, val)
#define VARIANT_FOR_EACH_FIELD(var, val)    LIST_FOR_EACH(var->fields, val)

snet_variant_t *SNetVariantCreate( snet_int_list_t *fields, snet_int_list_t *tags,
                                   snet_int_list_t *btags);

snet_variant_t *SNetVariantCreateEmpty();
snet_variant_t *SNetVariantCopy( snet_variant_t *var);
void SNetVariantDestroy( snet_variant_t *var);

void SNetVariantAddTag( snet_variant_t *var, int name);
void SNetVariantRemoveTag( snet_variant_t *var, int name);
bool SNetVariantHasTag( snet_variant_t *var, int name);
int SNetVariantNumTags( snet_variant_t *var);

void SNetVariantAddBTag( snet_variant_t *var, int name);
void SNetVariantRemoveBTag( snet_variant_t *var, int name);
bool SNetVariantHasBTag( snet_variant_t *var, int name);
int SNetVariantNumBTags( snet_variant_t *var);

void SNetVariantAddField( snet_variant_t *var, int name);
void SNetVariantRemoveField( snet_variant_t *var, int name);
bool SNetVariantHasField( snet_variant_t *var, int name);
int SNetVariantNumFields( snet_variant_t *var);
#endif
