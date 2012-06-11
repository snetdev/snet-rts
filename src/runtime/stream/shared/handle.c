/*
 * This implements the handle.
 */

#include <stdarg.h>
#include <string.h>

#include "handle_p.h"

#include "memfun.h"
#include "bool.h"


void SNetHndDestroy(snet_handle_t *hnd) {

  int i;
  if(hnd->mapping != NULL) {
    for(i=0; i<hnd->mapping->num; i++) {
      SNetMemFree(hnd->mapping->string_names[i]);
    }
    SNetMemFree(hnd->mapping->string_names);
    SNetMemFree(hnd->mapping->int_names);
  }
  SNetVariantListDestroy(hnd->vars);
}

/**
 * Should be public
 */
snet_record_t *SNetHndGetRecord(snet_handle_t *hnd)
{ 
  return hnd->rec;
}

snet_int_list_list_t *SNetHndGetVariants(snet_handle_t *hnd)
{
  return hnd->sign;
}

snet_variant_list_t *SNetHndGetVariantList(snet_handle_t *hnd)
{
  return(hnd->vars);
}

void SNetHndSetStringNames(snet_handle_t *hnd, int num, ...)
{
  int i;
  va_list args;

  va_start(args, num);
 
  hnd->mapping = (name_mapping_t *) SNetMemAlloc(sizeof(name_mapping_t));
  hnd->mapping->num = num;
  hnd->mapping->int_names = (int *) SNetMemAlloc(num * sizeof(int));
  hnd->mapping->string_names = (char **) SNetMemAlloc(num * sizeof(char*));

  for(i=0; i<num; i++) {
    char *str;
    int len;

    hnd->mapping->int_names[i] = va_arg(args, int);
    str = va_arg(args, char*);
    len = strlen(str)+1;
    hnd->mapping->string_names[i] = (char *) SNetMemAlloc(len * sizeof(char));
    (void) strncpy(hnd->mapping->string_names[i], str, len);
    hnd->mapping->string_names[i][len-1] = '\0';
  }
}

void *SNetHndGetExeRealm( snet_handle_t *hnd)
{
  return hnd->cdata;
}

void SNetHndSetExeRealm( snet_handle_t *hnd, void *cdata)
{
  hnd->cdata = cdata;
}
