
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

void C4SNet_free( void *ptr) 
{
  void  (*freefun)(void*) = ((C_Data*)ptr)->freefun;
 
  freefun( ((C_Data*)ptr)->data);

  SNetMemFree( ptr);
}


void *C4SNet_copy( void *ptr)
{
  C_Data *new;
  void* (*copyfun)(void*) = ((C_Data*)ptr)->copyfun;

  new = SNetMemAlloc( sizeof( C_Data));
  new->copyfun = ((C_Data*)ptr)->copyfun;
  new->freefun = ((C_Data*)ptr)->freefun;
  new->serfun = ((C_Data*)ptr)->serfun;
  new->encfun = ((C_Data*)ptr)->encfun;
  new->data = copyfun( ((C_Data*)ptr)->data);

  return( new);
}

int C4SNet_serialize(FILE *file, void *ptr)
{
  int (*serfun)(FILE *, void*) = ((C_Data*)ptr)->serfun;

  return( serfun( file, ((C_Data*)ptr)->data) );
}

int C4SNet_encode(FILE *file, void *ptr)
{
  int (*encfun)(FILE *, void*) = ((C_Data*)ptr)->encfun;

  return( encfun( file, ((C_Data*)ptr)->data) );
}

/*
void *C4SNet_deserialize(FILE *file)
{                      
  void *(*fun)(FILE *) = SNetGetDeserializationFun(my_interface_id);

  return fun(file);
}
 
void *C4SNet_decode(FILE *file)
{                      
  void *(*fun)(FILE *) = SNetGetDecodingFun(my_interface_id);

  return fun(file);
}
*/

void C4SNetInit( int id)
{
  my_interface_id = id;
  SNetGlobalRegisterInterface( id, &C4SNet_free, &C4SNet_copy,
			       &C4SNet_serialize, 
			       &C4SNet_deserialize,
			       &C4SNet_encode, 
			       &C4SNet_decode);  
}



/* ************************************************************************* */

C_Data *C4SNet_cdataCreate( void *data, 
			    void (*freefun)( void*),
			    void* (*copyfun)( void*),
			    int (*serfun)( FILE *, void*),
			    int (*encfun)( FILE *, void*))
{
  C_Data *c;

  c = malloc( sizeof( C_Data));
  c->data = data;
  c->freefun = freefun;
  c->copyfun = copyfun;
  c->serfun = serfun;
  c->encfun = encfun;

  return( c);
}

void *C4SNet_cdataGetData( C_Data *c)
{
  return( c->data);
}

void *C4SNet_cdataGetCopyFun( C_Data *c)
{
  return( c->copyfun);
}

void *C4SNet_cdataGetFreeFun( C_Data *c)
{ 
  return( c->freefun);
}

void *C4SNet_cdataGetSerializationFun( C_Data *c)
{ 
  return( c->serfun);
}

void *C4SNet_cdataGetEncodingFun( C_Data *c)
{ 
  return( c->encfun);
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


