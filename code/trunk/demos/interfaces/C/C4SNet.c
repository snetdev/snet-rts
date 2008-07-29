#include <stdlib.h>
#include <stdio.h>
#include "C4SNet.h"
#include "memfun.h"
#include "typeencode.h"
#include "interface_functions.h"
#include "out.h"
#include "base64.h"

#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

#define BUF_SIZE 32

static int interface_id;

struct container {
  snet_handle_t *hnd;
  int variant;
  
  int *counter;
  void **fields;
  int *tags;
  int *btags;
};

typedef union primary_types {
  unsigned char uc;
  char c;
  unsigned short us;
  short s;
  unsigned int ui;
  int i;
  unsigned long ul;
  long l;
  float f;
  double d;
  long double ld;
} c4snet_primary_type_t;

struct cdata {
  unsigned int ref_count;
  c4snet_type_t type;
  c4snet_primary_type_t data;
};

static int C4SNetSizeof(c4snet_type_t type){
  switch(type) {
  case CTYPE_uchar: 
    return sizeof(unsigned char);
    break;
  case CTYPE_char: 
    return sizeof(signed char);
    break;
  case CTYPE_ushort: 
    return sizeof(unsigned short);
    break;
  case CTYPE_short: 
    return sizeof(signed short);
    break;
  case CTYPE_uint: 
    return sizeof(unsigned int);
    break;
  case CTYPE_int: 
    return sizeof(signed int);
    break;
  case CTYPE_ulong:
    return sizeof(unsigned long);
    break;
  case CTYPE_long:
    return sizeof(signed long);
    break;
  case CTYPE_float: 
    return sizeof(float);
    break;
  case CTYPE_double: 
    return sizeof(double);
    break;
  case CTYPE_ldouble: 
    return sizeof(long double);
    break;
  case CTYPE_unknown:
  default:
    return 0;
    break;
  }
}

/***************************** Common functions ****************************/

void C4SNetInit( int id)
{
  interface_id = id;
  SNetGlobalRegisterInterface( id, 
			       &C4SNetDataFree, 
			       &C4SNetDataCopy,
			       &C4SNetDataSerialize, 
			       &C4SNetDataDeserialize,
			       &C4SNetDataEncode, 
			       &C4SNetDataDecode);  
}

void C4SNetOut( void *hnd, int variant, ...)
{
  va_list args;
  /*
  int i;
  void **fields;
  int *tags, *btags;
  snet_variantencoding_t *v;


  v = SNetTencGetVariant( SNetHndGetType( hnd), variant);
  fields = SNetMemAlloc( SNetTencGetNumFields( v) * sizeof( void*));
  tags = SNetMemAlloc( SNetTencGetNumTags( v) * sizeof( int));
  btags = SNetMemAlloc( SNetTencGetNumBTags( v) * sizeof( int));

  va_start( args, variant);
  for( i=0; i<SNetTencGetNumFields( v); i++) {
    fields[i] =  va_arg( args, void*);
  }
  for( i=0; i<SNetTencGetNumTags( v); i++) {
    tags[i] =  va_arg( args, int);
  }
  for( i=0; i<SNetTencGetNumBTags( v); i++) {
    btags[i] =  va_arg( args, int);
  }
  va_end( args);
*/
  va_start( args, variant);
  SNetOutRawV( hnd, interface_id, variant, args);
  va_end( args);
}



/****************************** Data functions *****************************/

c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, void *data)
{
  c4snet_data_t *c = NULL;

  int size = C4SNetSizeof(type);

  if(size != 0) {
    c = malloc( sizeof( c4snet_data_t));
    c->type = type;
    c->ref_count = 1;

    memcpy((void *)&c->data, data, size);
  }

  return( c);
}

void *C4SNetDataCopy( void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  temp->ref_count++;

  return ptr;
}

void C4SNetDataFree( void *ptr)
{
  
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  temp->ref_count--;

  if(temp->ref_count == 0) {
    SNetMemFree( ptr);
  }
}

void *C4SNetDataGetData( c4snet_data_t *c)
{
  if(c != NULL) {
    return (void *)&c->data;
  } 

  return c;
}

c4snet_type_t C4SNetDataGetType( c4snet_data_t *c) {
  if(c != NULL) {
    return( c->type);
  }
  return CTYPE_unknown;
}

int C4SNetDataSerialize( FILE *file, void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  switch(temp->type) {
  case CTYPE_uchar: 

    /* '&' and '<' must be encoded to '&amp;' and 'lt'; 
     * as they are not legal characters in  XML character data!
     */
    
    if((char)(temp->data.uc) == '<') {
      return fprintf(file, "(unsigned char)&lt;");
    }
    if((char)(temp->data.uc) == '&') {
      return fprintf(file, "(unsigned char)&amp;");
    }
    return fprintf(file, "(unsigned char)%c", (char)(temp->data.uc));
    break;
  case CTYPE_char: 

    /* '&' and '<' must be encoded to '&amp;' and 'lt'; 
     * as they are not legal characters in  XML character data!
     */

    if((char)(temp->data.c) == '<') {
      return fprintf(file, "(char)&lt;");
    }
    if((char)(temp->data.c) == '&') {
      return fprintf(file, "(char)&amp;");
    }   
    return fprintf(file, "(char)%c", (char)(temp->data.c));
    break;
  case CTYPE_ushort: 
    return fprintf(file, "(unsigned short)%hu", (unsigned short)temp->data.us);
    break;
  case CTYPE_short: 
    return fprintf(file, "(short)%hd", (short)temp->data.s);
    break;
  case CTYPE_uint: 
    return fprintf(file, "(unsigned int)%u", (unsigned int)temp->data.ui);
    break;
  case CTYPE_int: 
    return fprintf(file, "(int)%d", (int)temp->data.i);
    break;
  case CTYPE_ulong:
    return fprintf(file, "(unsigned long)%lu", (unsigned long)temp->data.ul);
    break;
  case CTYPE_long:
     return fprintf(file, "(long)%ld", (long)temp->data.l);
    break;
  case CTYPE_float: 
    return fprintf(file, "(float)%.32e", (double)temp->data.f);	
    break;
  case CTYPE_double: 
    return fprintf(file, "(double)%.32le", (double)temp->data.d);
    break;
  case CTYPE_ldouble: 
    return fprintf(file, "(long double)%.32le", (double)temp->data.d);
    break;
  default:
    return 0;
    break;
  }

  return 0;
}

void *C4SNetDataDeserialize(FILE *file) 
{
  char buf[BUF_SIZE];
  int j;
  char c;
  c4snet_primary_type_t t;

  j = 0;
  c = '\0';
  while((c = fgetc(file)) != EOF) {
    buf[j++] = c;
    if(c == '<') {
      ungetc('<', file);
      return NULL;
    }
    if(c == ')') {
      break;
    }
    if(j >= BUF_SIZE) {
      return NULL;
    }
  } 

  buf[j] = '\0';

  if(strcmp(buf, "(unsigned char)") == 0) {
    if(fscanf(file, "%c", &t.c) == 1) {

      /* '&' and '<' have been encoded to '&amp;' and 'lt'; 
       * as they are not legal characters in  XML character data!
       */

      if(t.c == '&') {
	j = 0;
	c = '\0';
	while((c = fgetc(file)) != EOF) {
	  buf[j++] = c;
	  if(c == '<') {
	    ungetc('<', file);
	    return NULL;
	  }
	  if(c == ';') {
	    break;
	  }
	  if(j >= BUF_SIZE) {
	    return NULL;
	  }
	} 

	buf[j] = '\0';

	if(strcmp(buf, "amp;") == 0) {
	  t.c = '&';
	}
	else if(strcmp(buf, "lt;") == 0) {
	  t.c = '<';
	}else{
	  return NULL;
	}
      } 
      else if(t.c == '<') {
	ungetc('<', file);
	return NULL;
      }

      t.uc = (unsigned char)t.c;
      return C4SNetDataCreate( CTYPE_uchar, &t.uc);
    }
  } 
  else if(strcmp(buf, "(char)") == 0) {
    if(fscanf(file, "%c", &t.c) == 1) {

      /* '&' and '<' have been encoded to '&amp;' and 'lt'; 
       * as they are not legal characters in  XML character data!
       */

       if(t.c == '&') {
	j = 0;
	c = '\0';
	while((c = fgetc(file)) != EOF) {
	  buf[j++] = c;
	  if(c == '<') {
	    ungetc('<', file);
	    return NULL;
	  }
	  if(c == ';') {
	    break;
	  }
	  if(j >= BUF_SIZE) {
	    return NULL;
	  }
	} 

	buf[j] = '\0';
	printf("data: %s\n", buf);
	
	if(strcmp(buf, "amp;") == 0) {
	  t.c = '&';
	} 
	else if(strcmp(buf, "lt;") == 0) {
	  t.c = '<';
	}else{
	  return NULL;
	}
      }
      else if(t.c == '<') {
	ungetc('<', file);
	return NULL;
      }

      return C4SNetDataCreate( CTYPE_char, &t.c);
    }
  }
  else if(strcmp(buf, "(unsigned short)") == 0) {
    if( fscanf(file, "%hu", &t.us) == 1) {
      return C4SNetDataCreate( CTYPE_ushort, &t.us);
    }
  }
  else if(strcmp(buf, "(short)") == 0) {
    if( fscanf(file, "%hd", &t.s) == 1) {
      return C4SNetDataCreate( CTYPE_short, &t.s);
    }
  }
  else if(strcmp(buf, "(unsigned int)") == 0) {
    if( fscanf(file, "%u", &t.ui) == 1) {
      return C4SNetDataCreate( CTYPE_uint, &t.ui);
    }
  }
  else if(strcmp(buf, "(int)") == 0) {
    if( fscanf(file, "%d", &t.i) == 1) {
      return C4SNetDataCreate( CTYPE_int, &t.i);
    }
  }
  else if(strcmp(buf, "(unsigned long)") == 0) {
    if( fscanf(file, "%lu", &t.ul) == 1) {
      return C4SNetDataCreate( CTYPE_ulong, &t.ul);
    }
  }
  else if(strcmp(buf, "(long)") == 0) {
    if( fscanf(file, "%ld", &t.l) == 1) {
      return C4SNetDataCreate( CTYPE_long, &t.l);
    }
  }
  else if(strcmp(buf, "(float)") == 0) {
    if( fscanf(file, "%f", &t.f) == 1) {
      return C4SNetDataCreate( CTYPE_float, &t.f);
    }
  }
  else if(strcmp(buf, "(double)") == 0) {
    if( fscanf(file, "%lf", &t.d) == 1) {
      return C4SNetDataCreate( CTYPE_double, &t.d);
    }
  } 
  else if(strcmp(buf, "(long double)") == 0) {
    if( fscanf(file, "%lf", &t.d) == 1) {
      return C4SNetDataCreate( CTYPE_ldouble, &t.d);
    }
  }
 
  return NULL;
}

int C4SNetDataEncode( FILE *file, void *ptr){
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  int i = 0;

  i = Base64encodeDataType(file, (int)temp->type);

  i += Base64encode(file, (unsigned char *)&temp->data, C4SNetSizeof(temp->type));

  return i + 1;
}

void *C4SNetDataDecode(FILE *file) 
{
  c4snet_data_t *c = NULL;
  int i = 0;
  c4snet_type_t type;
  int size;

  i = Base64decodeDataType(file, (int *)&type);

  size = C4SNetSizeof(type);

  if(size != 0) {
    c = malloc( sizeof( c4snet_data_t));
    c->type = type;
    c->ref_count = 1;

    i += Base64decode(file, (unsigned char *)&c->data, size);
  }

  return c;
}


/**************************** Container functions ***************************/

c4snet_container_t *C4SNetContainerCreate( void *hnd, int var_num) 
{
  
  int i;
  c4snet_container_t *c;
  snet_variantencoding_t *v;


  v = SNetTencGetVariant( 
        SNetTencBoxSignGetType( SNetHndGetBoxSign( hnd)), var_num);

  c = SNetMemAlloc( sizeof( c4snet_container_t));
  c->counter = SNetMemAlloc( 3 * sizeof( int));
  c->fields = SNetMemAlloc( SNetTencGetNumFields( v) * sizeof( void*));
  c->tags = SNetMemAlloc( SNetTencGetNumTags( v) * sizeof( int));
  c->btags = SNetMemAlloc( SNetTencGetNumBTags( v) * sizeof( int));
  c->hnd = hnd;
  c->variant = var_num;

  for( i=0; i<3; i++) {
    c->counter[i] = 0;
  }

  return( c);
}



c4snet_container_t *C4SNetContainerSetField(c4snet_container_t *c, void *ptr)
{
  c->fields[ F_COUNT( c)] = ptr;
  F_COUNT( c) += 1;

  return( c);
}


c4snet_container_t *C4SNetContainerSetTag(c4snet_container_t *c, int val)
{
  c->tags[ T_COUNT( c)] = val;
  T_COUNT( c) += 1;

  return( c);
}


c4snet_container_t *C4SNetContainerSetBTag(c4snet_container_t *c, int val)
{
  c->btags[ B_COUNT( c)] = val;
  B_COUNT( c) += 1;

  return( c);
}

void C4SNetContainerOut(c4snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, interface_id, c->variant, c->fields, c->tags, c->btags);
  
  SNetMemFree(c->counter);
  SNetMemFree(c);
}


#undef BUF_SIZE
#undef F_COUNT
#undef T_COUNT
#undef B_COUNT
