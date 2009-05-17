
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SAC4SNet.h"
#include "snetentities.h"
#include "memfun.h"
#include "typeencode.h"
#include "out.h"

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


/*
 * Serialisation & Deserialisation functions 
 */
static int SAC4SNetDataSerialise( FILE *file, void *ptr);
static void *SAC4SNetDataDeserialise(FILE *file);
static int SAC4SNetDataEncode( FILE *file, void *ptr);
static void *SAC4SNetDataDecode(FILE *file);

/******************************************************************************/

void SAC4SNet_outCompound( sac4snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, my_interface_id, c->variant, c->fields, c->tags, c->btags);
}

void SAC4SNet_out( void *hnd, int variant, ...)
{
  int i;
  void **fields;
  int *tags, *btags;
  snet_variantencoding_t *venc;
  snet_vector_t *mapping;
  int num_entries, f_count=0, t_count=0, b_count=0;
  int *f_names, *t_names, *b_names;
  va_list args;


  venc = SNetTencGetVariant(
           SNetTencBoxSignGetType( SNetHndGetBoxSign( hnd)), variant);
  fields = SNetMemAlloc( SNetTencGetNumFields( venc) * sizeof( void*));
  tags = SNetMemAlloc( SNetTencGetNumTags( venc) * sizeof( int));
  btags = SNetMemAlloc( SNetTencGetNumBTags( venc) * sizeof( int));

  num_entries = SNetTencGetNumFields( venc) +
                SNetTencGetNumTags( venc) +
                SNetTencGetNumBTags( venc);
  mapping = 
    SNetTencBoxSignGetMapping( SNetHndGetBoxSign( hnd), variant-1); 
  f_names = SNetTencGetFieldNames( venc);
  t_names = SNetTencGetTagNames( venc);
  b_names = SNetTencGetBTagNames( venc);

  va_start( args, variant);
  for( i=0; i<num_entries; i++) {
    switch( SNetTencVectorGetEntry( mapping, i)) {
      case field:
        fields[f_count++] =  SACARGnewReference( va_arg( args, SACarg*));
        break;
      case tag:
        tags[t_count++] =  va_arg( args, int);
        break;
      case btag:
        btags[b_count++] = va_arg( args, int);
        break;
    }
  }
  va_end( args);

  SNetOutRawArray( hnd, my_interface_id, variant, fields, tags, btags);
}

void SAC4SNetInit( int id)
{
  my_interface_id = id;
  SNetGlobalRegisterInterface( id, 
                               &SACARGfree, 
                               &SACARGcopy, 
                               &SAC4SNetDataSerialise, 
                               NULL,
                               NULL,
                               NULL);  
}

/******************************************************************************/


static int SAC4SNetDataSerialise( FILE *file, void *ptr)
{
  SACarg *arg = (SACarg*)ptr;
  
  fprintf( file, "%d", SACARGgetBasetype( arg));

  return( 0);
}


/******************************************************************************/

sac4snet_container_t *SAC4SNet_containerCreate( void *hnd, int var_num) 
{
  
  int i;
  sac4snet_container_t *c;
  snet_variantencoding_t *v;


  v = SNetTencGetVariant( SNetHndGetType( hnd), var_num);

  c = SNetMemAlloc( sizeof( sac4snet_container_t));
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



sac4snet_container_t *SAC4SNet_containerSetField( sac4snet_container_t *c, void *ptr)
{
  c->fields[ F_COUNT( c)] = ptr;
  F_COUNT( c) += 1;

  return( c);
}


sac4snet_container_t *SAC4SNet_containerSetTag( sac4snet_container_t *c, int val)
{
  c->tags[ T_COUNT( c)] = val;
  T_COUNT( c) += 1;

  return( c);
}


sac4snet_container_t *SAC4SNet_containerSetBTag( sac4snet_container_t *c, int val)
{
  c->btags[ B_COUNT( c)] = val;
  B_COUNT( c) += 1;

  return( c);
}



