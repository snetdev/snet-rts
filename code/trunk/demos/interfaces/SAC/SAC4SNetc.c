#include <stdlib.h>
#include "SAC4SNetc.h"



#define MAX_ARGS 99
#define STRAPPEND( dest, src)     dest = strappend( dest, src)

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


static char *strappend( char *dest, char *src)
{
  char *res;
  res = realloc( dest, (strlen( dest) + strlen( src) + 1) * sizeof( char));
  strcat( res, src);

  return( res);
}

static void constructSACfqn( char **fqn, char *box_name, void *meta_data)
{
  /* TODO: use meta-data to construct function name */
  *fqn = malloc( (strlen( box_name) + 1) * sizeof( char));
  strcpy( *fqn, box_name);
}

char *SAC4SNetGenBoxWrapper( char *box_name,
                             snet_input_type_enc_t *t,
                             void *meta_data) 
{
  int i;
  char *wrapper_code, *tmp_str, *sac_fqn;

  /* construct full SAC function name */
  constructSACfqn( &sac_fqn,  box_name, meta_data);

  /* generate prototype of SAC function */
  wrapper_code = 
    malloc( ( strlen( "extern void ") + 1) * sizeof( char)); 
  strcpy( wrapper_code, "extern void ");

  STRAPPEND( wrapper_code, sac_fqn);
  STRAPPEND( wrapper_code, "( SACarg **hnd_return, SACarg *hnd");
  for( i=0; i<t->num; i++) {
    STRAPPEND( wrapper_code, ", SACarg *arg_");
    tmp_str = itoa( i);
    STRAPPEND( wrapper_code, tmp_str);
    free( tmp_str);
  }
  STRAPPEND( wrapper_code, ");\n\n");
  
  /* generate box wrapper */
  STRAPPEND( wrapper_code, "static void *SNetCall__");
  STRAPPEND( wrapper_code, box_name);
  STRAPPEND( wrapper_code, "( void *handle");
  for( i=0; i<t->num; i++) {
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

  /* box wrapper body */
  STRAPPEND( wrapper_code, "  SACarg *hnd_return, *hnd;\n\n");
  STRAPPEND( wrapper_code, "  hnd = SACARGconvertFromVoidPointer( SACTYPE"
                           "_SNet_SNet, handle);\n");
  STRAPPEND( wrapper_code, "  ");
  STRAPPEND( wrapper_code, sac_fqn);
  STRAPPEND( wrapper_code, "( &hnd_return, hnd");
  for( i=0; i<t->num; i++) {
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
  STRAPPEND( wrapper_code, ");\n\n"
            "  return( "
            "SACARGconvertToVoidPointer( SACTYPE_SNet_SNet, hnd_return));\n");
  
  STRAPPEND( wrapper_code, "}\n");
   
  free( sac_fqn);
  return( wrapper_code);
}



