#include <stdlib.h>
#include "C4SNetWrap.h"



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
  str = malloc( len * sizeof( char));
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

char *C4SNetGenBoxWrapper( char *box_name,
                           snet_input_type_enc_t *t,
                           void *meta_data) 
{
  int i;
  char *wrapper_code, *tmp_str, *c_fqn;

  /* construct full SAC function name */
  c_fqn = box_name;

  /* generate prototype of SAC function */
  wrapper_code = 
    malloc( ( strlen( "extern void *") + 1) * sizeof( char)); 
  strcpy( wrapper_code, "extern void *");

  STRAPPEND( wrapper_code, c_fqn);
  STRAPPEND( wrapper_code, "( void *hnd");
  for( i=0; i<t->num; i++) {
    switch( t->type[i]) {
      case field:
        STRAPPEND( wrapper_code, ", void *arg_");
        break;
      case tag:
      case btag:
        STRAPPEND( wrapper_code, ", int arg_");
        break;
      }
      tmp_str = itoa( i);
      STRAPPEND( wrapper_code, tmp_str);
      free( tmp_str);
  }
  STRAPPEND( wrapper_code, ");\n\n");
  
  /* generate box wrapper */
  STRAPPEND( wrapper_code, "void *SNetCall__");
  STRAPPEND( wrapper_code, box_name);
  STRAPPEND( wrapper_code, "( void *handle");
  for( i=0; i<t->num; i++) {
    switch( t->type[i]) {
      case field:
        STRAPPEND( wrapper_code, ", void *");
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
  STRAPPEND( wrapper_code, "  return( ");
  STRAPPEND( wrapper_code, c_fqn);
  STRAPPEND( wrapper_code, "( handle");
  for( i=0; i<t->num; i++) {
    STRAPPEND( wrapper_code, ", arg_");
    tmp_str = itoa( i);
    STRAPPEND( wrapper_code, tmp_str);
  }
  
  STRAPPEND( wrapper_code, "));\n}\n");

  return( wrapper_code);
}



