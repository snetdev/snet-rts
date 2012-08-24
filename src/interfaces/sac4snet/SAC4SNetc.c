#include <stdlib.h>
#include "SAC4SNetc.h"



#define MAX_ARGS 99
#define STRAPPEND(dest, src)     dest = strappend(dest, src)

/** Define SAC specific strings and recognised meta-data keys **/
#define MOD_DELIMITER "__" 
#define TYPE_STRING   "SACTYPE_SNet_SNet"

#define MOD_NAME      "SACmodule"
#define BOX_FUN       "SACboxfun"
#define OUTVIARETURN  "SACoutViaReturn"
#define DEFAULT_MAP   "SACdefaultmap"

/** ********************************************************* **/


static char *itoa(int val)
{
  int i;
  int len=0;
  int offset=0;
  char *result;
  char *str;

  result = malloc((MAX_ARGS + 1) * sizeof(char));
  result[MAX_ARGS] = '\0';

  offset=(int)'0';
  for(i=0; i<MAX_ARGS; i++)
  {
    result[MAX_ARGS-1-i]=(char)((val%10)+offset);
    val/=10;
    if(val==0)
    {
      len=(i+1);
      break;
    }
   }
  str = malloc((len+1) * sizeof(char));
  strcpy(str,&result[MAX_ARGS-len]);
  free(result);

  return(str);
}


static char *strappend(char *dest, const char *src)
{
  char *res;
  res = realloc(dest, (strlen(dest) + strlen(src) + 1) * sizeof(char));
  strcat(res, src);

  return(res);
}

static void constructSACfqn(char **fqn, 
                             char *box_name, 
                             int num_args,
                             snet_meta_data_enc_t *meta_data)
{
  const char *key_val;

  *fqn = malloc(sizeof(char));
  *fqn[0] = '\0';

  key_val = SNetMetadataGet(meta_data, MOD_NAME);
  if(key_val != NULL) {
    STRAPPEND(*fqn, key_val);
    STRAPPEND(*fqn, MOD_DELIMITER);
    key_val = SNetMetadataGet(meta_data, BOX_FUN);
    if(key_val != NULL) {
      STRAPPEND(*fqn, key_val);
    }
    else {
      STRAPPEND(*fqn, box_name);
    }
    STRAPPEND(*fqn, itoa(num_args+1));
  }
  else {
    STRAPPEND(*fqn, box_name);
  }
}

char *SAC4SNetGenBoxWrapper(char *box_name,
                             snetc_type_enc_t *t,
                             int num_outtypes,
                             snetc_type_enc_t **out_types,
                             snet_meta_data_enc_t *meta_data) 
{
  int i;
  int arg_num;
  char *wrapper_code, *tmp_str, *sac_fqn;
  /* a default task-mapping information can be placed in the metadata */
  const char *defmap = SNetMetadataGet(meta_data, DEFAULT_MAP);
  /* for now, the presence of the default mapping implies a multithreaded sac box */
  const int is_mtbox = (defmap != NULL);
  
  arg_num = (t == NULL) ? 0 : t->num;

  /* construct full SAC function name */
  constructSACfqn(&sac_fqn, box_name, arg_num, meta_data);


  /* generate prototype of SAC function */
  wrapper_code = 
    malloc((strlen("extern void ") + 1) * sizeof(char)); 
  strcpy(wrapper_code, "extern void ");

  STRAPPEND(wrapper_code, sac_fqn);
  STRAPPEND(wrapper_code, "(SACarg **hnd_return, SACarg *hnd");
  for(i=0; i<arg_num; i++) {
    STRAPPEND(wrapper_code, ", SACarg *arg_");
    tmp_str = itoa(i);
    STRAPPEND(wrapper_code, tmp_str);
    free(tmp_str);
  }
  STRAPPEND(wrapper_code, ");\n\n");

  /* execution realm functions */
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_create__");
  STRAPPEND(wrapper_code, box_name);
  STRAPPEND(wrapper_code, "(snet_handle_t *h)\n{\n"); 
  if (is_mtbox) {
    STRAPPEND(wrapper_code, "  SAChive *hive;\n");
    if (defmap[0] == '$') {
      /* oooh, that's gona be interesting. The default task mapping is supposed to be
       * available at runtime in an environment variable */
      STRAPPEND(wrapper_code, "  const char *s_defmap = getenv(\"");
      STRAPPEND(wrapper_code, &defmap[1]);    // skip the $
      STRAPPEND(wrapper_code, "\");\n");
      /* handle missing variable as an error */
      STRAPPEND(wrapper_code, "  if (s_defmap == NULL) {\n");
      STRAPPEND(wrapper_code, "    SNetUtilDebugFatal(\"In SNetExeRealm_create__");
      STRAPPEND(wrapper_code, box_name);
      STRAPPEND(wrapper_code, ": the variable '");
      STRAPPEND(wrapper_code, &defmap[1]);    // skip the $
      STRAPPEND(wrapper_code, "' not found in the environment!\");\n");
      STRAPPEND(wrapper_code, "  }\n");
      /* determine the number of ints in the CSV string */
      STRAPPEND(wrapper_code, "  const int defmap_num = SAC4SNetParseIntCSV(s_defmap, NULL);\n");
      /* alloc an array and parse the CSV into it */
      STRAPPEND(wrapper_code, "  int *defmap = calloc(defmap_num, sizeof(int));\n");
      STRAPPEND(wrapper_code, "  SAC4SNetParseIntCSV(s_defmap, defmap);\n");
      /* debug print */
      STRAPPEND(wrapper_code, "  SAC4SNetDebugPrintMapping(\"The box '");
      STRAPPEND(wrapper_code, box_name);
      STRAPPEND(wrapper_code, "' is mapped to \", defmap, defmap_num);\n");
    } else {
      STRAPPEND(wrapper_code, "  static const int defmap[] = { ");
      STRAPPEND(wrapper_code, defmap);
      STRAPPEND(wrapper_code, " };\n");
      STRAPPEND(wrapper_code, "  const int defmap_num = sizeof(defmap) / sizeof(int);\n\n");
    }
    STRAPPEND(wrapper_code, "  hive = SAC_AllocHive(defmap_num, 2, defmap, h);\n");
    if (defmap[0] == '$') {
      STRAPPEND(wrapper_code, "  free(defmap); defmap = NULL;\n");
    }
    STRAPPEND(wrapper_code, "  assert(hive != NULL);\n");
    STRAPPEND(wrapper_code, "  SNetHndSetExeRealm(h, hive);\n");
  }
  STRAPPEND(wrapper_code, "  return(h);");
  STRAPPEND(wrapper_code, "\n}\n\n");
  
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_update__");
  STRAPPEND(wrapper_code, box_name);
  STRAPPEND(wrapper_code, "(snet_handle_t *h) { return(h); }\n\n");
  
  STRAPPEND(wrapper_code, "static snet_handle_t *SNetExeRealm_destroy__");
  STRAPPEND(wrapper_code, box_name);
  STRAPPEND(wrapper_code, "(snet_handle_t *h)\n{\n");
  if (is_mtbox) {
    STRAPPEND(wrapper_code, "  SAC_ReleaseHive(SNetHndGetExeRealm(h));\n");  
    STRAPPEND(wrapper_code, "  SNetHndSetExeRealm(h, NULL);\n");
  }
  STRAPPEND(wrapper_code, "  return(h); \n}\n\n");
  
  /* generate box wrapper */
  STRAPPEND(wrapper_code, "static void *SNetCall__");
  STRAPPEND(wrapper_code, box_name);
  STRAPPEND(wrapper_code, "(snet_handle_t *handle");
  for(i=0; i<arg_num; i++) {
    switch(t->type[i]) {
      case field:
        STRAPPEND(wrapper_code, ", SACarg *");
        break;
      case tag:
      case btag:
        STRAPPEND(wrapper_code, ", int ");
        break;
    }
    STRAPPEND(wrapper_code, "arg_");
    tmp_str = itoa(i);
    STRAPPEND(wrapper_code, tmp_str);
    free(tmp_str);
  }
  STRAPPEND(wrapper_code, ")\n{\n");

  
  /* box wrapper body */
  STRAPPEND(wrapper_code, "  SACarg *hnd = SACARGconvertFromVoidPointer(SACTYPE"
                           "_SNet_SNet, handle);\n");
  STRAPPEND(wrapper_code, "  SACarg *hnd_return;\n");
  
  /* retrieve execution realm if in mt mode */
  if (is_mtbox) {
    STRAPPEND(wrapper_code, "  SAChive *hive = SNetHndGetExeRealm(handle);\n\n");
    STRAPPEND(wrapper_code, "  SAC_AttachHive(hive);\n");
    STRAPPEND(wrapper_code, "  SNetHndSetExeRealm(handle, NULL);\n\n");
  }

  STRAPPEND(wrapper_code, "  ");
  STRAPPEND(wrapper_code, sac_fqn);
  STRAPPEND(wrapper_code, "(&hnd_return, hnd");
  for(i=0; i<arg_num; i++) {
      switch(t->type[i]) {
      case field:
        STRAPPEND(wrapper_code, ", arg_");
        break;
      case tag:
      case btag:
        STRAPPEND(wrapper_code, ", SACARGconvertFromIntScalar(arg_");
        break;
    }
    tmp_str = itoa(i);
    STRAPPEND(wrapper_code, tmp_str);
    switch(t->type[i]) {
      case tag:
      case btag:
        STRAPPEND(wrapper_code, ")");
        break;
      default:
        break;
    }
  }
  STRAPPEND(wrapper_code, ");\n\n");
  
  if (is_mtbox) {
    STRAPPEND(wrapper_code,
        "  hive = SAC_DetachHive();\n"
        "  SNetHndSetExeRealm(handle, hive);\n"
        "\n");
  }
  
  STRAPPEND(wrapper_code, "  return(SACARGconvertToVoidPointer(");
  STRAPPEND(wrapper_code, TYPE_STRING);
  STRAPPEND(wrapper_code, ", hnd_return));\n");
  
  STRAPPEND(wrapper_code, "}\n");
   
  free(sac_fqn);

  return(wrapper_code);
}



