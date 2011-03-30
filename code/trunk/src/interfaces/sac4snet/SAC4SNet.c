/** <!--********************************************************************-->
 *
 * $Id$
 *
 * SAC4SNet is a the SAC language interface for S-Net. 
 *
 * Serialisation of data is done using the FibreIO module of SAC's stdlib. 
 * The wrappers for required functions are implemented in SAC4SNetFibreIO.sac. 
 * We use these functions here to conveniently create SACargs from input data
 * and to serialise SAC arrays to fibre format. As SAC's fibre functions 
 * require dimensionality and shape information, serialised data is prefixed
 * with a header. Data that is read in for deserialisation is expected to be
 * prefixed with this header as well.
 *
 * The header is defined as 
 *
 * FIBREHEADER = IDSTRINGPRE BTYPE DIM SHP IDSTRINGSUF
 * BTYPE       = int
 *             | double
 *             | float
 * DIM         = num
 * SHP         = num [SHP]*
 *
 * where BTYPE is any of above shown  strings (it is converted to the number
 * that is reported by SACARGgetBasetype()), DIM is the dimensionality
 * (SACARGgetDim()) and SHP is a list of DIM integers (SACARGgetShape(arg, 0)
 * ... SACARGgetShape( arg, DIM-1)).  IDSTRINGPRE and IDSTRINGSUF are defined
 * below.
 *
 *
 * Note 1: De-/Serialisation currently only works for int, float, double
 *         ,i.e. for those base types for which FibreIO functions exist.
 * Note 2: MPI related functions are not yet implemented. 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SAC4SNet.h"
#include "sac_include/SAC4SNetFibreIO.h"
#include "snetentities.h"
#include "memfun.h"
#include "typeencode.h"
#include "out.h"

#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

#define IDSTRINGPRE ":::SACFIBRE:"
#define IDSTRINGSUF ":::"

const char *t_descr[] = {
#define TYP_IFdb_str( n) n
#include "type_info.mac" 
#undef TYP_IFdb_str
};

int my_interface_id;

typedef enum { SACint=3, SACflt=11, SACdbl=12} SACbasetype_t; 

extern void SACARGfree( void*);
extern void *SACARGcopy( void*);

struct container {
  struct handle *hnd;
  int variant;
  
  int *counter;
  void **fields;
  int *tags;
  int *btags;
};

/*
 * Abort execution on error
 */
static void Error( char *msg)
{
  printf("\n\n** SAC4SNet Fatal Error ** (abort) : %s\n\n", msg);
  exit( -1);
}

static int TstrToNum( char *s)
{
  int i = 0;
  
  /* T_nothing is the last entry of the types table */
  while( strcmp( t_descr[i], s) != 0) {
    if( strcmp( t_descr[i], "nothing") == 0) {
      Error( "Invalid Basetype");
    } 
    i += 1;
  }
  return( i);
}

static int MaxTNum()
{
  int i = 0;
  while( strcmp( t_descr[i++], "nothing") != 0);
  return( i-1);
}



/*
 * Serialisation & Deserialisation functions 
 */
static int SAC4SNetDataSerialise( FILE *file, void *ptr);
static void *SAC4SNetDataDeserialise(FILE *file);
static int SAC4SNetDataEncode( FILE *file, void *ptr)
{
  Error( "DataEncoding not implemented.");
  return( 0);
}
static void *SAC4SNetDataDecode(FILE *file)
{
  Error( "DataDecoding not implemented.");
  return( 0);
}

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
  SNetInterfaceRegister( id,
                         &SACARGfree,
                         &SACARGcopy,
                         &SAC4SNetDataSerialise,
                         &SAC4SNetDataDeserialise,
                         &SAC4SNetDataEncode,
                         &SAC4SNetDataDecode);
}

/******************************************************************************/


static int SAC4SNetDataSerialise( FILE *file, void *ptr)
{
  int i, dim;
  SACarg *ret, *arg = (SACarg*)ptr;
  char *basetype_str;
  int btype;

  btype = SACARGgetBasetype( arg);

  if( btype <= MaxTNum()) {
    basetype_str = SNetMemAlloc( 
                      (strlen( t_descr[ btype]) + 1) * sizeof( char));
    strcpy( basetype_str, t_descr[ btype]);
  } 
  else {
    basetype_str = SNetMemAlloc( 
                      (strlen( "(UDT)") + 1) * sizeof( char));
    strcpy( basetype_str, "(UDT)");
  }
   
  if( arg != NULL) { 

    dim = SACARGgetDim( arg);
    fprintf( file, "\n%s %s %d ", IDSTRINGPRE, 
                                  basetype_str, 
                                  dim);
    for( i=0; i<dim; i++) {
      fprintf( file, "%d ",SACARGgetShape( arg, i));
    }
    fprintf(file, " %s\n", IDSTRINGSUF);

    switch( SACARGgetBasetype( arg)) {
      case SACint: 
        SAC4SNetFibreIO__PrintIntArray2( 
            &ret,
            SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
            SACARGnewReference( arg));
      break;
      case SACflt: 
        SAC4SNetFibreIO__PrintFloatArray2( 
            &ret,
            SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
            SACARGnewReference( arg));
      break;
      case SACdbl: 
        SAC4SNetFibreIO__PrintDoubleArray2( 
            &ret,
            SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
            SACARGnewReference( arg));
      break;
      default:
        fprintf( file, "## UNSERIALISABLE BASETYPE (%d) ##\n", btype);
      break;
    }

  }
  SNetMemFree( basetype_str);

  return( 0);
}

static void *SAC4SNetDataDeserialise( FILE *file)
{
  int i;
  char buf[128], tstr[128];
  char datafile_name[128];
  FILE *datafile;

  int basetype, dim, *shape;
  SACarg *scanres, *sac_shp, *dummy;

  fscanf( file, "%s", buf); 

  if( strcmp( IDSTRINGPRE, buf) == 0) {
    fscanf( file, "%s%d", tstr, &dim);
    basetype = TstrToNum ( tstr);
    shape = SNetMemAlloc( dim * sizeof( int));
    for( i=0; i<dim; i++) {
      fscanf( file, "%d", &shape[i]);
    }
    fscanf( file, "%s", buf);
    
    /* This is a temporary workaround to avoid 
     * having two yaccs accessing the same file (which results
     * in interesting behaviour...)
     */
    fscanf( file, "%s", datafile_name);
    datafile = fopen( datafile_name, "r");
    if( datafile == NULL) {
      Error( "Failed to open datafile");
    }
    
    if( strcmp( IDSTRINGSUF, buf) == 0) {
      sac_shp = SACARGconvertFromIntPointerVect( shape, 1, &dim);
      switch( basetype) {
        case SACint:
          SAC4SNetFibreIO__ScanIntArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, datafile),
              sac_shp);
          break;
        case SACdbl:
          SAC4SNetFibreIO__ScanDoubleArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, datafile),
              sac_shp);
          break;
        case SACflt:
          SAC4SNetFibreIO__ScanFloatArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, datafile),
              sac_shp);
          break;
        default: /* unsupported base type */
            scanres = NULL;
            printf( "\n>%d<\n", basetype);
            Error("Unsupported Base Type");
          break;
      }
      fclose( datafile);
    }
    else { /* Illegal Header Suffix */
      scanres = NULL;
      Error( "Illegal Header Suffix"); 
    }
  } 
  else { /* Illegal Header Prefix */
    scanres = NULL;
      Error( "Illegal Header Prefix"); 
  }

  return( (void*)scanres);
}
/******************************************************************************/

sac4snet_container_t *SAC4SNet_containerCreate( struct handle *hnd, snet_typeencoding_t *type, int var_num) 
{
  
  int i;
  sac4snet_container_t *c;
  snet_variantencoding_t *v;


  v = SNetTencGetVariant( type, var_num);

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
