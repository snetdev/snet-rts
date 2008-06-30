
#include <stdlib.h>
#include <stdio.h>
#include "C4SNet.h"
#include "C4SNetText.h"
#include "memfun.h"
#include "typeencode.h"
#include "interface_functions.h"
#include "out.h"
#include "base64.h"
#include <pthread.h>

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

static int C4SNetSizeof(C4SNet_type_t type){
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

void C4SNetFree( void *ptr)
{
  
  C4SNet_data_t *temp = (C4SNet_data_t *)ptr;

  temp->ref_count--;

  if(temp->ref_count == 0) {
    SNetMemFree( ptr);
  }
}

void *C4SNetCopy( void *ptr)
{
  C4SNet_data_t *temp = (C4SNet_data_t *)ptr;

  temp->ref_count++;

  return ptr;
}

int C4SNetEncode( FILE *file, void *ptr){
  C4SNet_data_t *temp = (C4SNet_data_t *)ptr;

  int i = 0;

  i = Base64encodeDataType(file, (int)temp->type);

  i += Base64encode(file, (unsigned char *)&temp->data, C4SNetSizeof(temp->type));

  return i + 1;
}

void *C4SNetDecode(FILE *file) 
{
  C4SNet_data_t *c = NULL;
  int i = 0;
  C4SNet_type_t type;
  int size;

  i = Base64decodeDataType(file, (int *)&type);

  size = C4SNetSizeof(type);

  if(size != 0) {
    c = malloc( sizeof( C4SNet_data_t));
    c->type = type;
    c->ref_count = 1;

    i += Base64decode(file, (unsigned char *)&c->data, size);
  }

  return c;
}

void C4SNet_outCompound( c4snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, my_interface_id, c->variant, c->fields, c->tags, c->btags);
  
  SNetMemFree(c->counter);
  SNetMemFree(c);
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

C4SNet_data_t *C4SNet_cdataCreate( C4SNet_type_t type, void *data)
{
  C4SNet_data_t *c = NULL;

  int size = C4SNetSizeof(type);

  if(size != 0) {
    c = malloc( sizeof( C4SNet_data_t));
    c->type = type;
    c->ref_count = 1;

    memcpy((void *)&c->data, data, size);
  }

  return( c);
}

void *C4SNet_cdataGetData( C4SNet_data_t *c)
{
  if(c != NULL) {
    return (void *)&c->data;
  } 

  return c;
}

C4SNet_type_t C4SNet_cdataGetType( C4SNet_data_t *c) {
  if(c != NULL) {
    return( c->type);
  }
  return CTYPE_unknown;
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


