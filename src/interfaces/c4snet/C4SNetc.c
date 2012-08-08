#include <stdlib.h>
#include <stdio.h>
#include "C4SNetc.h"

#define MAX_NUM                   50
#define STRAPPEND( dest, src)     dest = strappend(dest, src)

static char *strappend(char *dest, const char *src)
{
  char *res;

  res = malloc(strlen(dest) + strlen(src) + 1);
  strcpy(res, dest);
  strcat(res, src);
  free(dest);

  return res;
}

char *C4SNetGenBoxWrapper( char *box_name,
                           snetc_type_enc_t *t,
                           int num_out_types,
                           snetc_type_enc_t **out_types,
                           snet_meta_data_enc_t *meta_data)
{
  char *wrapper_code, *c_fqn, num_str[MAX_NUM];
  (void) num_out_types; /* NOT USED */
  (void) out_types; /* NOT USED */
  (void) meta_data; /* NOT USED */

  /* construct full C function name */
  c_fqn = box_name;

  /* generate prototype of C function */
  wrapper_code = strdup("extern void *");

  STRAPPEND(wrapper_code, c_fqn);
  STRAPPEND(wrapper_code, "(void *hnd");
  if (t != NULL) {
    for (int i = 0; i < t->num; i++) {
      switch (t->type[i]) {
        case field:
          STRAPPEND(wrapper_code, ", void *arg_");
          break;
        case tag:
        case btag:
          STRAPPEND(wrapper_code, ", int arg_");
          break;
        }
        snprintf(num_str, MAX_NUM, "%d", i);
        STRAPPEND(wrapper_code, num_str);
    }
  }
  STRAPPEND(wrapper_code, ");\n\n");

  /* execution realms are not used for C4SNet */
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_create__");
  STRAPPEND(wrapper_code, c_fqn);
  STRAPPEND(wrapper_code, "(snet_handle_t *h) { return(h); }\n\n");
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_update__");
  STRAPPEND(wrapper_code, c_fqn);
  STRAPPEND(wrapper_code, "(snet_handle_t *h) { return(h); }\n\n");
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_destroy__");
  STRAPPEND(wrapper_code, c_fqn);
  STRAPPEND(wrapper_code, "(snet_handle_t *h) { return(h); }\n\n");

  /* generate box wrapper */
  STRAPPEND(wrapper_code, "snet_handle_t *SNetCall__");
  STRAPPEND(wrapper_code, box_name);
  STRAPPEND(wrapper_code, "(snet_handle_t *handle");
  if (t != NULL) {
    for (int i = 0; i < t->num; i++) {
      switch (t->type[i]) {
        case field:
          STRAPPEND(wrapper_code, ", void *");
          break;
        case tag:
        case btag:
          STRAPPEND(wrapper_code, ", int ");
          break;
      }
      STRAPPEND(wrapper_code, "arg_");
      snprintf(num_str, MAX_NUM, "%d", i);
      STRAPPEND(wrapper_code, num_str);
    }
  }
  STRAPPEND(wrapper_code, ")\n{\n");

  /* box wrapper body */
  STRAPPEND(wrapper_code, "  return ");
  STRAPPEND(wrapper_code, c_fqn);
  STRAPPEND(wrapper_code, "(handle");
  if (t != NULL) {
    for (int i = 0; i < t->num; i++) {
      STRAPPEND(wrapper_code, ", arg_");
      snprintf(num_str, MAX_NUM, "%d", i);
      STRAPPEND(wrapper_code, num_str);
    }
  }
  STRAPPEND(wrapper_code, ");\n}");

  return wrapper_code;
}



