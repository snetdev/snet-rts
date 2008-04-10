
#include <stdlib.h>
#include <SAC2SNet.h>
#include <snetentities.h>
#include <memfun.h>
#include <typeencode.h>
#include <stdio.h>

#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

extern void SACARGfree( void*);
extern void *SACARGcopy( void*);


int my_interface_id;

struct container {
  snet_handle_t *hnd;
  int variant;
  
  int *counter;
  void **fields;
  int *tags;
  int *btags;
};

void SAC2SNet_outCompound( sac2snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, my_interface_id, c->variant, c->fields, c->tags, c->btags);
}

void SAC2SNet_out( void *hnd, int variant, ...)
{
  int i;
  void **fields;
  int *tags, *btags;
  snet_variantencoding_t *v;
  va_list args;


  v = SNetTencGetVariant( SNetHndGetType( hnd), variant);
  fields = SNetMemAlloc( SNetTencGetNumFields( v) * sizeof( void*));
  tags = SNetMemAlloc( SNetTencGetNumTags( v) * sizeof( int));
  btags = SNetMemAlloc( SNetTencGetNumBTags( v) * sizeof( int));

  va_start( args, variant);
  for( i=0; i<SNetTencGetNumFields( v); i++) {
    fields[i] =  SACARGnewReference( va_arg( args, SACarg*));
  }
  for( i=0; i<SNetTencGetNumTags( v); i++) {
    tags[i] =  va_arg( args, int);
  }
  for( i=0; i<SNetTencGetNumBTags( v); i++) {
    btags[i] = va_arg( args, int);
  }
  va_end( args);

  SNetOutRawArray( hnd, my_interface_id, variant, fields, tags, btags);
}

void SAC2SNet_init( int id)
{
  my_interface_id = id;
  SNetGlobalRegisterInterface( id, &SACARGfree, &SACARGcopy, NULL, NULL);  
}


sac2snet_container_t *SAC2SNet_containerCreate( void *hnd, int var_num) 
{
  
  int i;
  sac2snet_container_t *c;
  snet_variantencoding_t *v;


  v = SNetTencGetVariant( SNetHndGetType( hnd), var_num);

  c = SNetMemAlloc( sizeof( sac2snet_container_t));
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



sac2snet_container_t *SAC2SNet_containerSetField( sac2snet_container_t *c, void *ptr)
{
  c->fields[ F_COUNT( c)] = ptr;
  F_COUNT( c) += 1;

  return( c);
}


sac2snet_container_t *SAC2SNet_containerSetTag( sac2snet_container_t *c, int val)
{
  c->tags[ T_COUNT( c)] = val;
  T_COUNT( c) += 1;

  return( c);
}


sac2snet_container_t *SAC2SNet_containerSetBTag( sac2snet_container_t *c, int val)
{
  c->btags[ B_COUNT( c)] = val;
  B_COUNT( c) += 1;

  return( c);
}



