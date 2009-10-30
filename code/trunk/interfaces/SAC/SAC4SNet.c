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

#ifdef DISTRIBUTED_SNET
#include <mpi.h>
#endif /* DISTRIBUTED_SNET */

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

typedef enum { SACint=1, SACflt=7, SACdbl=8} SACbasetype_t; 

typedef struct { int basetype; int dim; int *shape;} sac4snet_typeinf_t;

#define TIbasetype( n) n->basetype
#define TIdim( n) n->dim
#define TIshape( n) n->shape

static sac4snet_typeinf_t *TIcreate( int dim)
{
  sac4snet_typeinf_t *ti = SNetMemAlloc( sizeof ( sac4snet_typeinf_t));
  TIshape( ti) = SNetMemAlloc( dim * sizeof( int));
  return( ti);
}

static void *TIdestroy( sac4snet_typeinf_t *ti)
{
  SNetMemFree( TIshape( ti));
  SNetMemFree( ti);
  return( NULL);
}
  


extern void SACARGfree( void*);
extern void *SACARGcopy( void*);


struct container {
  snet_handle_t *hnd;
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
#ifdef DISTRIBUTED_SNET
static int SAC4SNetSerializeType( MPI_Comm comm, 
                                  void *data, 
                                  void *buf, 
                                  int size);

static int SAC4SNetPack( MPI_Comm comm, 
                         void *data, 
                         MPI_Datatype *type, 
                         void **buf, 
                         void **opt);

static void SAC4SNetCleanup( MPI_Datatype type, 
                             void *opt);

static int SAC4SNetDeserializeType( MPI_Comm comm, 
                                    void *buf, 
                                    int size, 
                                    MPI_Datatype *type, 
                                    void **opt);

static void* SAC4SNetUnpack( MPI_Comm comm, 
                             void *buf, 
                             MPI_Datatype type, 
                             int count, 
                             void *opt);
#endif /* DISTRIBUTED_SNET */


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
#                            ifdef DISTRIBUTED_SNET
                               SNET_interface_MPI,
#                            else 
			                         SNET_interface_basic,
#                            endif
                               &SACARGfree, 
                               &SACARGcopy, 
                               &SAC4SNetDataSerialise, 
                               &SAC4SNetDataDeserialise,
                               &SAC4SNetDataEncode,
#                            ifdef DISTRIBUTED_SNET
                               &SAC4SNetDataDecode,
                               &SAC4SNetSerializeType,
                               &SAC4SNetDeserializeType,
                               &SAC4SNetPack,
                               &SAC4SNetUnpack,
                               &SAC4SNetCleanup
#                            else
                               &SAC4SNetDataDecode
#                            endif
                               );  
}

/******************************************************************************/


static int SAC4SNetDataSerialise( FILE *file, void *ptr)
{
  int i, dim;
  SACarg *ret, *arg = (SACarg*)ptr;

  if( arg != NULL) { 

    dim = SACARGgetDim( arg);
    fprintf( file, "\n%s %s %d ", IDSTRINGPRE, 
                                  t_descr[SACARGgetBasetype( arg)], 
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
        fprintf( file, "##UNSUPPORTED BASETYPE##");
      break;
    }

  }

  return( 0);
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

static void *SAC4SNetDataDeserialise( FILE *file)
{
  int i;
  char buf[128], tstr[128];
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
    if( strcmp( IDSTRINGSUF, buf) == 0) {
      sac_shp = SACARGconvertFromIntPointerVect( shape, 1, &dim);
      switch( basetype) {
        case SACint:
          SAC4SNetFibreIO__ScanIntArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
              sac_shp);
          break;
        case SACdbl:
          SAC4SNetFibreIO__ScanDoubleArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
              sac_shp);
          break;
        case SACflt:
          SAC4SNetFibreIO__ScanFloatArray2( 
              &dummy, 
              &scanres, 
              SACARGconvertFromVoidPointer( SACTYPE_StdIO_File, file),
              sac_shp);
          break;
        default: /* unsupported base type */
            scanres = NULL;
            Error("Unsupported Base Type");
          break;
      }
    }
    else { /* Illegal Header Suffix */
      scanres = NULL;
      Error( "Illegal Header Suffix"); 
    }
  } 
  else { /* Illegal Header Prefix*/
    scanres = NULL;
      Error( "Illegal Header Prefix"); 
  }

  return( (void*)scanres);
}

/******************************************************************************/


#ifdef DISTRIBUTED_SNET
static MPI_Datatype SAC4SNetBasetypeToMPIType( int basetype)
{
  switch( basetype)
  {
    case SACint:
      return( MPI_INT);
      break;
    case SACflt:
      return( MPI_FLOAT);
      break;
    case SACdbl:
      return( MPI_DOUBLE);
      break;
    default:
      Error( "Unsupported basetype in type conversion.");
      break;
  }
  return( MPI_DATATYPE_NULL);
}

static int SAC4SNetSerializeType(MPI_Comm comm, void *data, void *buf, int size)
{
  SACarg *temp = (SACarg *)data;
  int pack_size;
  int position = 0;
  int dims, type, *shape;
  int i;

  /* We pack the following type information:
   * 1 int  - basetype
   * 1 int  - dimensionality n
   * n ints - size of dims 0 .. n-1
   */

  dims = SACARGgetDim( temp);
  type = SACARGgetBasetype( temp);
  shape = SNetMemAlloc( dims * sizeof( int));
  for( i=0; i<dims; i++) {
    shape[i] = SACARGgetShape( temp, i);  
  }
  
  MPI_Pack_size( 2+dims, MPI_INT, comm, &pack_size);

  if(pack_size < size) {
    MPI_Pack( &type, 1, MPI_INT, buf, size, &position, comm);
    MPI_Pack( &dims, 1, MPI_INT, buf, size, &position, comm);
    MPI_Pack( shape, dims, MPI_INT, buf, size, &position, comm);
  } else {
    position = SNET_INTERFACE_ERR;
  }

  return position;
}

static int SAC4SNetPack(MPI_Comm comm, void *data, MPI_Datatype *type, void **buf, void **opt)
{
  SACarg *tmp = (SACarg *)data;
  int i;
  int basetype = SACARGgetBasetype( tmp);
  int num_elems = 1;

  *type = SAC4SNetBasetypeToMPIType( basetype);

  for( i=0; i<SACARGgetDim( tmp); i++) {
    num_elems *= SACARGgetShape( tmp, i);
  }

  switch( basetype) {
    case SACint:
      *buf = SACARGconvertToIntArray( tmp);
      break;
    case SACdbl:
      *buf = SACARGconvertToDoubleArray( tmp);
      break;
    case SACflt:
      *buf = SACARGconvertToFloatArray( tmp);
      break;
    default:
      Error( "Unsupported basetype in pack function");
      break;
  }

  return num_elems;
}

static void SAC4SNetCleanup(MPI_Datatype type, void *opt)
{
  /* currently nothing to do! */
}

static int C4SNetDeserializeType(MPI_Comm comm, void *buf, int size, MPI_Datatype *type, void **opt)
{
  int position = 0;
  int i;
  int basetype;
  int dims;
  int num_elems = 1;
  sac4snet_typeinf_t *ti;

  
  MPI_Unpack( buf, size, &position, &basetype, 1, MPI_INT, comm);
  MPI_Unpack( buf, size, &position, &dims, 1, MPI_INT, comm);

  ti = TIcreate( dims);
  TIbasetype( ti) = basetype;

  MPI_Unpack( buf, size, &position, &TIshape( ti), dims, MPI_INT, comm);

  for( i=0; i<dims; i++) {
    num_elems *= TIshape( ti)[i];
  }

  *opt = ti;

  *type = SAC4SNetBasetypeToMPIType( basetype);

  return num_elems;
}

static void* SAC4SNetUnpack(MPI_Comm comm, void *buf, MPI_Datatype type, int count, void *opt)
{
  SACarg *t;
  sac4snet_typeinf_t *ti = (sac4snet_typeinf_t *)opt;
  switch( TIbasetype( ti)) {
    case SACint:
      t = SACARGconvertFromIntPointerVect( buf, TIdim( ti), TIshape( ti));
      break;
    case SACflt:
      t = SACARGconvertFromFloatPointerVect( buf, TIdim( ti), TIshape( ti));
      break;
    case SACdbl:
      t = SACARGconvertFromDoublePointerVect( buf, TIdim( ti), TIshape( ti));
      break;
    default:
      Error( "Unsupported basetype in unpack function");
      break;
  }
  
  ti = TIdestroy( ti);

  return t;
}
#endif

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




