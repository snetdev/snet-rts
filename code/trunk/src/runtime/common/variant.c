#include "memfun.h"
#include "variant.h"
#include "list.h"

struct snet_variant {
  snet_int_list_t *tags, *btags, *fields;
};

snet_variant_t *SNetVariantCreate( snet_int_list_t *fields,
                                   snet_int_list_t *tags,
                                   snet_int_list_t *btags)
{
  snet_variant_t *variant = SNetMemAlloc(sizeof(snet_variant_t));

  variant->tags= tags;
  variant->btags= btags;
  variant->fields = fields;

  return variant;
}

snet_variant_t *SNetVariantCreateEmpty()
{
  snet_variant_t *variant = SNetMemAlloc(sizeof(snet_variant_t));

  variant->tags = SNetintListCreate(0, -1);
  variant->btags = SNetintListCreate(0, -1);
  variant->fields = SNetintListCreate(0, -1);

  return variant;
}

snet_variant_t *SNetVariantCopy( snet_variant_t *var)
{
  return SNetVariantCreate( SNetintListCopy(var->fields),
                            SNetintListCopy(var->tags),
                            SNetintListCopy(var->btags));
}

void SNetVariantDestroy( snet_variant_t *var)
{
  SNetintListDestroy(var->tags);
  SNetintListDestroy(var->btags);
  SNetintListDestroy(var->fields);
  SNetMemFree(var);
}



void SNetVariantAddTag( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetintListAppend(var->tags, name);
}

void SNetVariantRemoveTag( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetintListRemove(var->tags, name);
}

bool SNetVariantHasTag( snet_variant_t *var, int name)
{
  return SNetintListContains(var->tags, name);
}

int SNetVariantNumTags( snet_variant_t *var)
{
  return SNetintListSize(var->tags);
}



void SNetVariantAddBTag( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetintListAppend(var->btags, name);
}

void SNetVariantRemoveBTag( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetintListRemove(var->btags, name);
}

bool SNetVariantHasBTag( snet_variant_t *var, int name)
{
  return SNetintListContains(var->btags, name);
}

int SNetVariantNumBTags( snet_variant_t *var)
{
  return SNetintListSize(var->btags);
}



void SNetVariantAddField( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetintListAppend(var->fields, name);
}

void SNetVariantRemoveField( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetintListRemove(var->fields, name);
}

bool SNetVariantHasField( snet_variant_t *var, int name)
{
  return SNetintListContains(var->fields, name);
}

int SNetVariantNumFields( snet_variant_t *var)
{
  return SNetintListSize(var->fields);
}
