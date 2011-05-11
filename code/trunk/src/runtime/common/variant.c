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

  variant->tags = SNetIntListCreate(0);
  variant->btags = SNetIntListCreate(0);
  variant->fields = SNetIntListCreate(0);

  return variant;
}

snet_variant_t *SNetVariantCopy( snet_variant_t *var)
{
  return SNetVariantCreate( SNetIntListCopy(var->fields),
                            SNetIntListCopy(var->tags),
                            SNetIntListCopy(var->btags));
}

void SNetVariantDestroy( snet_variant_t *var)
{
  SNetIntListDestroy(var->tags);
  SNetIntListDestroy(var->btags);
  SNetIntListDestroy(var->fields);
  SNetMemFree(var);
}



void SNetVariantAddTag( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetIntListAppend(var->tags, name);
}

void SNetVariantRemoveTag( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetIntListRemove(var->tags, name);
}

bool SNetVariantHasTag( snet_variant_t *var, int name)
{
  return SNetIntListContains(var->tags, name);
}

int SNetVariantNumTags( snet_variant_t *var)
{
  return SNetIntListLength(var->tags);
}



void SNetVariantAddBTag( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetIntListAppend(var->btags, name);
}

void SNetVariantRemoveBTag( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetIntListRemove(var->btags, name);
}

bool SNetVariantHasBTag( snet_variant_t *var, int name)
{
  return SNetIntListContains(var->btags, name);
}

int SNetVariantNumBTags( snet_variant_t *var)
{
  return SNetIntListLength(var->btags);
}



void SNetVariantAddField( snet_variant_t *var, int name)
{
  //FIXME: Already exists?
  return SNetIntListAppend(var->fields, name);
}

void SNetVariantRemoveField( snet_variant_t *var, int name)
{
  //FIXME: not present?
  SNetIntListRemove(var->fields, name);
}

bool SNetVariantHasField( snet_variant_t *var, int name)
{
  return SNetIntListContains(var->fields, name);
}

int SNetVariantNumFields( snet_variant_t *var)
{
  return SNetIntListLength(var->fields);
}
