#include <stdlib.h>
#include "SAC4SNetc.h"



#define MAX_ARGS 99
#define STRAPPEND( dest, src)     dest = strappend( dest, src)

/** Define SAC specific strings and recognised meta-data keys **/
#define MOD_DELIMITER "__" 
#define TYPE_STRING   "SACTYPE_SNet_SNet"

#define MOD_NAME      "SACmodule"
#define BOX_FUN       "SACboxfun"
#define OUTVIARETURN  "SACoutViaReturn"
#define DEFAULT_MAP   "SACdefaultmap"

/** ********************************************************* **/


static char *itoa( int val)
{
  int i;
  int len=0;
  int offset=0;
  char *result;
  char *str;

  result = malloc( (MAX_ARGS + 1) * sizeof( char));
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
  str = malloc( (len+1) * sizeof( char));
  strcpy(str,&result[MAX_ARGS-len]);
  free( result);

  return( str);
}


static char *strappend( char *dest, const char *src)
{
  char *res;
  res = realloc( dest, (strlen( dest) + strlen( src) + 1) * sizeof( char));
  strcat( res, src);

  return( res);
}

static void constructSACfqn( char **fqn, 
                             char *box_name, 
                             int num_args,
                             snet_meta_data_enc_t *meta_data)
{
  const char *key_val;

  *fqn = malloc( sizeof( char));
  *fqn[0] = '\0';

  key_val = SNetMetadataGet( meta_data, MOD_NAME);
  if( key_val != NULL) {
    STRAPPEND( *fqn, key_val);
    STRAPPEND( *fqn, MOD_DELIMITER);
    key_val = SNetMetadataGet( meta_data, BOX_FUN);
    if( key_val != NULL) {
      STRAPPEND( *fqn, key_val);
    }
    else {
      STRAPPEND( *fqn, box_name);
    }
    STRAPPEND( *fqn, itoa( num_args+1));
  }
  else {
    STRAPPEND( *fqn, box_name);
  }
}

char *SAC4SNetGenBoxWrapper( char *box_name,
                             snetc_type_enc_t *t,
                             int num_outtypes,
                             snetc_type_enc_t **out_types,
                             snet_meta_data_enc_t *meta_data) 
{
  int i;
  int arg_num;
  char *wrapper_code, *tmp_str, *sac_fqn;

  arg_num = (t == NULL) ? 0 : t->num;

  /* construct full SAC function name */
  constructSACfqn( &sac_fqn, box_name, arg_num, meta_data);


  /* generate prototype of SAC function */
  wrapper_code = 
    malloc( ( strlen( "extern void ") + 1) * sizeof( char)); 
  strcpy( wrapper_code, "extern void ");

  STRAPPEND( wrapper_code, sac_fqn);
  STRAPPEND( wrapper_code, "( SACarg **hnd_return, SACarg *hnd");
  for( i=0; i<arg_num; i++) {
    STRAPPEND( wrapper_code, ", SACarg *arg_");
    tmp_str = itoa( i);
    STRAPPEND( wrapper_code, tmp_str);
    free( tmp_str);
  }
  STRAPPEND( wrapper_code, ");\n\n");
  
  /* generate box wrapper */
  STRAPPEND( wrapper_code, "static void *SNetCall__");
  STRAPPEND( wrapper_code, box_name);
  STRAPPEND( wrapper_code, "( snet_handle_t *handle");
  for( i=0; i<arg_num; i++) {
    switch( t->type[i]) {
      case field:
        STRAPPEND( wrapper_code, ", SACarg *");
        break;
      case tag:
      case btag:
        STRAPPEND( wrapper_code, ", int ");
        break;
    }
    STRAPPEND( wrapper_code, "arg_");
    tmp_str = itoa( i);
    STRAPPEND( wrapper_code, tmp_str);
    free( tmp_str);
  }
  STRAPPEND( wrapper_code, ")\n{\n");

  /* a default task-mapping information can be placed in the metadata */
  const char *defmap = SNetMetadataGet( meta_data, DEFAULT_MAP);
  /* for now, the presence of the default mapping implies a multithreaded sac box */
  const int is_mtbox = (defmap != NULL);
  
  if (is_mtbox) {
    STRAPPEND( wrapper_code, "  /* default mapping can be given via the meta-data */\n");
    if (defmap) {
      /* insert the default mapping as-is */
      STRAPPEND( wrapper_code, "  static const int defmap[] = { ");
      STRAPPEND( wrapper_code, defmap);
      STRAPPEND( wrapper_code, " };\n");
      STRAPPEND( wrapper_code, "  const int defmap_num = sizeof(defmap) / sizeof(int);\n\n");
    } else {
      /* no default mapping */
      //STRAPPEND( wrapper_code, "  const int *defmap = NULL;\n\n");
    }
    
    STRAPPEND( wrapper_code,
        "  /* retrieve my continuation: a handle to a bee-hive */\n"
        "  SAChive *hive = SNetHndGetCData(handle);\n"
        "\n");
    
    if (defmap) {
      STRAPPEND( wrapper_code,
        "  if (!hive) {\n"
        "    /* initialize the hive according to the default mapping */\n"
        "    hive = SAC_AllocHive(defmap_num, 2,\n"
        "                         defmap,   /* places */\n"
        "                         handle);   /* opaque thdata */\n"
        "  }\n"
        "\n");
    }
    
    STRAPPEND( wrapper_code,
        "  if (hive) {\n"
        "    SAC_AttachHive(hive);\n"
        "    SNetHndSetCData(handle, NULL);\n"
        "    hive = NULL;    /* the hive-handle is no longer valid */\n"
        "  }\n"
        "\n");
  }
  
  /* box wrapper body */
  STRAPPEND( wrapper_code, "  SACarg *hnd = SACARGconvertFromVoidPointer( SACTYPE"
                           "_SNet_SNet, handle);\n");
  STRAPPEND( wrapper_code, "  SACarg *hnd_return;\n");
  STRAPPEND( wrapper_code, "  ");
  STRAPPEND( wrapper_code, sac_fqn);
  STRAPPEND( wrapper_code, "( &hnd_return, hnd");
  for( i=0; i<arg_num; i++) {
      switch( t->type[i]) {
      case field:
        STRAPPEND( wrapper_code, ", arg_");
        break;
      case tag:
      case btag:
        STRAPPEND( wrapper_code, ", SACARGconvertFromIntScalar( arg_");
        break;
    }
    tmp_str = itoa( i);
    STRAPPEND( wrapper_code, tmp_str);
    switch( t->type[i]) {
      case tag:
      case btag:
        STRAPPEND( wrapper_code, ")");
        break;
      default:
        break;
    }
  }
  STRAPPEND( wrapper_code, ");\n\n");
  
  if (is_mtbox) {
    STRAPPEND( wrapper_code,
        "  /* pick up my continuation */\n"
        "  hive = SAC_DetachHive();\n"
        "  SAC_ReleaseHive(hive);\n"      /* FIXME: when the continuation destructor is ready, comment this... */
        "  //SNetHndSetCData(handle, hive);\n"    /*    ... and uncomment this */
        "\n");
  }
  
  STRAPPEND( wrapper_code, "  return( SACARGconvertToVoidPointer( ");
  STRAPPEND( wrapper_code, TYPE_STRING);
  STRAPPEND( wrapper_code, ", hnd_return));\n");
  
  STRAPPEND( wrapper_code, "}\n");
   
  free( sac_fqn);

  return( wrapper_code);
}



