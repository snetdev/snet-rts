
#include <stdlib.h>
#include "C4SNet.h"
#include "C4SNetTypes.h"
#include "memfun.h"
#include "typeencode.h"
#include "interface_functions.h"
#include "out.h"


#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

int my_interface_id;

struct container {
  snet_handle_t *hnd;
  int variant;
  
  int *counter;
  void **fields;
  int *tags;
  int *btags;
};

void C4SNet_outCompound( c4snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, my_interface_id, c->variant, c->fields, c->tags, c->btags);
}

void C4SNet_out( void *hnd, int variant, ...)
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
  SNetOutRawV( hnd, my_interface_id, variant, args);
  va_end( args);
}

void C4SNetInit( int id)
{
  my_interface_id = id;
  SNetGlobalRegisterInterface( id, 
			       &C4SNetFree, 
			       &C4SNetCopy,
			       &C4SNetSerialize, 
			       &C4SNetDeserialize,
			       &C4SNetEncode, 
			       &C4SNetDecode);  
}



/* ************************************************************************* */

C_Data *C4SNet_cdataCreate( ctype_t type, void *data)
{
  C_Data *c;
  c = malloc( sizeof( C_Data));
  c->type = type;

  switch(type) {
  case CTYPE_uchar: 
    c->data.uc = *(unsigned char *)data;
    break;
  case CTYPE_char: 
    c->data.c = *(char *)data;
    break;
  case CTYPE_ushort: 
    c->data.us = *(unsigned short *)data;
    break;
  case CTYPE_short: 
    c->data.s = *(short *)data;
    break;
  case CTYPE_uint: 
    c->data.ui = *(unsigned int *)data;
    break;
  case CTYPE_int: 
    c->data.i = *(int *)data;
    break;
  case CTYPE_ulong:
    c->data.ul = *(unsigned long *)data;
    break;
  case CTYPE_long:
    c->data.l = *(long *)data;
    break;
  case CTYPE_float: 
    c->data.f = *(float *)data;	
    break;
  case CTYPE_double: 
    c->data.d = *(double *)data;
    break;
  default:
    free(c);
    return NULL;
    break;
  }

  return( c);
}

void *C4SNet_cdataGetData( C_Data *c)
{
  C_Data *temp = (C_Data *)c;

  switch(temp->type) {
  case CTYPE_uchar: 
    return &temp->data.uc;
    break;
  case CTYPE_char: 
    return &temp->data.c;
    break;
  case CTYPE_ushort: 
    return &temp->data.us;
    break;
  case CTYPE_short: 
    return &temp->data.s;
    break;
  case CTYPE_uint: 
    return &temp->data.ui;
    break;
  case CTYPE_int: 
    return &temp->data.i;
    break;
  case CTYPE_ulong:
    return &temp->data.ul;
    break;
  case CTYPE_long:
     return &temp->data.l;
    break;
  case CTYPE_float: 
    return &temp->data.f;	
    break;
  case CTYPE_double: 
    return &temp->data.d;
    break;
  default:
    return NULL;
    break;
  }

  return NULL;
}

ctype_t C4SNet_cdataGetType( C_Data *c) {
  return( c->type);
}


/* ************************************************************************* */


c4snet_container_t *C4SNet_containerCreate( void *hnd, int var_num) 
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



c4snet_container_t *C4SNet_containerSetField( c4snet_container_t *c, void *ptr)
{
  c->fields[ F_COUNT( c)] = ptr;
  F_COUNT( c) += 1;

  return( c);
}


c4snet_container_t *C4SNet_containerSetTag( c4snet_container_t *c, int val)
{
  c->tags[ T_COUNT( c)] = val;
  T_COUNT( c) += 1;

  return( c);
}


c4snet_container_t *C4SNet_containerSetBTag( c4snet_container_t *c, int val)
{
  c->btags[ B_COUNT( c)] = val;
  B_COUNT( c) += 1;

  return( c);
}


