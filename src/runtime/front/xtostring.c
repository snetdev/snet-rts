#include <string.h>
#include "node.h"

/* The maximum length of a record, variant or variant list string. */
static const int string_buffer_size = 200;

/* Make a short description of a record type. */
void SNetRecordTypeString(snet_record_t *rec, char *buf, size_t size)
{
  snet_ref_t    *field;
  int            name, val;
  char          *label;
  size_t         len = 0;
  snetin_label_t*labels;

  if (REC_DESCR(rec) != REC_data) {
    snprintf(buf, size, "%s", SNetRecTypeName(rec));
    return;
  }

  labels = SNetObserverGetLabels();
  buf[len] = '\0';
  RECORD_FOR_EACH_FIELD(rec, name, field) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL){
      snprintf(&buf[len], size - len, "%c%s", len ? ',' : '{', label);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
    (void) field;
  }
  RECORD_FOR_EACH_TAG(rec, name, val) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL) {
      snprintf(&buf[len], size - len, "%c<%s=%d>", len ? ',' : '{', label, val);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
    (void) val;
  }
  RECORD_FOR_EACH_BTAG(rec, name, val) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL) {
      snprintf(&buf[len], size - len, "%c<#%s=%d>", len ? ',' : '{', label, val);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
    (void) val;
  }
  snprintf(&buf[len], size - len, "}");
}

/* Convert a record type to a dynamically allocated string. */
char *SNetGetRecordTypeString(snet_record_t *rec)
{
  char buf[string_buffer_size];

  SNetRecordTypeString(rec, buf, sizeof buf);

  return strdup(buf);
}

/* Make a short description of a variant type. */
void SNetVariantString(snet_variant_t *var, char *buf, size_t size)
{
  int            name;
  char          *label;
  size_t         len = 0;
  snetin_label_t*labels = SNetObserverGetLabels();

  buf[len] = '\0';
  VARIANT_FOR_EACH_FIELD(var, name) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL){
      snprintf(&buf[len], size - len, "%c%s", len ? ',' : '{', label);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
  }
  VARIANT_FOR_EACH_TAG(var, name) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL) {
      snprintf(&buf[len], size - len, "%c<%s>", len ? ',' : '{', label);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
  }
  VARIANT_FOR_EACH_BTAG(var, name) {
    if ((label = SNetInIdToLabel(labels, name)) != NULL) {
      snprintf(&buf[len], size - len, "%c<#%s>", len ? ',' : '{', label);
      len += strlen(&buf[len]);
    }
    SNetMemFree(label);
  }
  if (len == 0) {
    snprintf(&buf[len], size - len, "{");
    len += strlen(&buf[len]);
  }
  snprintf(&buf[len], size - len, "}");
}

/* Convert a variant type to a dynamically allocated string. */
char *SNetGetVariantString(snet_variant_t *var)
{
  char buf[string_buffer_size];

  SNetVariantString(var, buf, sizeof buf);

  return strdup(buf);
}

/* Make a short description of a variant list type. */
void SNetVariantListString(snet_variant_list_t *vl, char *buf, size_t size)
{
  snet_variant_t *other;
  size_t len = 0, count = 0;

  buf[len] = '\0';
  LIST_FOR_EACH(vl, other) {
    if (++count >= 2) {
      snprintf(&buf[len], size - len, "|");
      len += strlen(&buf[len]);
    }
    SNetVariantString(other, &buf[len], size - len);
    len += strlen(&buf[len]);
  }
}

/* Convert a variant list type to a dynamically allocated string. */
char *SNetGetVariantListString(snet_variant_list_t *vl)
{
  char buf[string_buffer_size];

  SNetVariantListString(vl, buf, sizeof buf);

  return strdup(buf);
}

