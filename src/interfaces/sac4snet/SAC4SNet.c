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
#include <assert.h>
#include <ctype.h>
#include "SAC4SNet.h"
#include "SAC4SNetFibreIO.h"
#include "snetentities.h"
#include "memfun.h"
#include "debug.h"
#include "variant.h"
#include "type.h"
#include "out.h"
#include "list.h"

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

#ifdef ENABLE_DIST_MPI
static void SAC4SNetMPIPackFun(void *cdata, void *buf);
static void *SAC4SNetMPIUnpackFun(void *buf);
#endif
#ifdef ENABLE_DIST_SCC
static void SAC4SNetSCCPackFun(void *cdata, void *dest);
static void *SAC4SNetSCCUnpackFun(void *localBuf);
#endif

struct container {
  struct handle *hnd;
  snet_variant_t *variant;

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
static void SAC4SNetDataSerialise( FILE *file, void *ptr);
static void *SAC4SNetDataDeserialise(FILE *file);
static void SAC4SNetDataEncode( FILE *file, void *ptr)
{
  Error( "DataEncoding not implemented.");
}
static void *SAC4SNetDataDecode(FILE *file)
{
  Error( "DataDecoding not implemented.");
  return( 0);
}

static size_t sizeOfType(SACbasetype_t type)
{
  switch (type) {
    case SACint: return sizeof(int);
    case SACdbl: return sizeof(double);
    case SACflt: return sizeof(float);
    default:
      SNetUtilDebugFatal("Unknown SAC type!\n");
      break;
  }

  return -1;
}

static size_t SAC4SNetAllocSize(void *data)
{
  int dims, num_elems = 1;

  dims = SACARGgetDim(data);
  for (int i = 0; i < dims; i++) {
    num_elems *= SACARGgetShape(data, i);
  }

  return num_elems * sizeOfType(SACARGgetBasetype(data));
}

/******************************************************************************/

void SAC4SNet_outCompound( sac4snet_container_t *c) 
{
  SNetOutRawArray( c->hnd, my_interface_id, c->variant, c->fields, c->tags, c->btags);
}

void SAC4SNet_out( void *hnd, int variant_num, ...)
{
  int i;
  void **fields;
  int *tags, *btags, name, f=0, t=0, b=0;
  snet_int_list_t *variant;
  snet_variant_t *v;
  va_list args;


  variant = SNetIntListListGet( SNetHndGetVariants( (struct handle *) hnd), variant_num-1);
  v = SNetVariantListGet( SNetHndGetVariantList( (struct handle*) hnd), variant_num-1);
  fields = SNetMemAlloc( SNetVariantNumFields( v) * sizeof( void*));
  tags = SNetMemAlloc( SNetVariantNumTags( v) * sizeof( int));
  btags = SNetMemAlloc( SNetVariantNumBTags( v) * sizeof( int));

  va_start( args, variant_num);
    for(i=0; i<SNetIntListLength( variant); i+=2) {
      name = SNetIntListGet( variant, i+1);
      switch(SNetIntListGet( variant, i)) {
        case field:
          fields[f++] =  SACARGnewReference( va_arg( args, SACarg*));
          break;
        case tag:
          tags[t++] =  va_arg( args, int);
          break;
        case btag:  
          btags[b++] =  va_arg( args, int);
          break;
        default: 
          assert(0);
      }
    }
  va_end( args);

  SNetOutRawArray( hnd, my_interface_id, v, fields, tags, btags);
}

void SAC4SNetInit( int id, snet_distrib_t distImpl)
{
  my_interface_id = id;
  snet_pack_fun_t packfun = NULL;
  snet_unpack_fun_t unpackfun = NULL;

  switch (distImpl) {
    case nodist:
      break;
    case mpi:
      #ifdef ENABLE_DIST_MPI
        packfun = &SAC4SNetMPIPackFun;
        unpackfun = &SAC4SNetMPIUnpackFun;
      #else
        SNetUtilDebugFatal("SAC4SNet supports MPI, but is not configured to use "
                           "it.\n");
      #endif
      break;
    case scc:
      #ifdef ENABLE_DIST_SCC
        packfun = &SAC4SNetSCCPackFun;
        unpackfun = &SAC4SNetSCCUnpackFun;
      #else
        SNetUtilDebugFatal("SAC4SNet supports SCC, but is not configured to use "
                           "it.\n");
      #endif
      break;
    default:
      SNetUtilDebugFatal("SAC4SNet doesn't support the selected distribution "
                         "layer (%d).\n", distImpl);
      break;
  }

  SNetInterfaceRegister( id,
                         &SACARGfree,
                         &SACARGcopy,
                         &SAC4SNetAllocSize,
                         &SAC4SNetDataSerialise,
                         &SAC4SNetDataDeserialise,
                         &SAC4SNetDataEncode,
                         &SAC4SNetDataDecode,
                         packfun,
                         unpackfun);
  
  SAC_InitRuntimeSystem();
}

/******************************************************************************/


static void SAC4SNetDataSerialise( FILE *file, void *ptr)
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

/*
 * A utility function to parse a comma separated string (CSV) of integers
 * into an array of ints. The format of @csv is, eg:
 *    1, 2, 3, 4
 * The function returns the total number of integers found in the string.
 * The values are returned in the optional @nums array.
 */
int SAC4SNetParseIntCSV(const char *csv, int *nums)
{
  int count = 0;    /* the number of numbers found */

  do {
    /* skip all whitespace and delimiters */
    while (isspace(*csv) || *csv == ',' || *csv == ';') {
      ++csv;
    }
    if (*csv == '\0') {
      /* end of string */
      break;
    }

    char *invalid;    /* ptr to the first invalid character in csv */
    int v = strtol(csv, &invalid, 10);
    if (invalid == csv) {
      /* nonsensical character at the position, no number found */
      Error("SAC4SNetParseIntCSV: the csv string contains an invalid character!");
    }

    /* there was a number */
    if (nums != NULL) {
      nums[count] = v;
    }
    /* inc the count of numbers */
    ++count;
    /* reposition at the first invalid character, which might be just a whitespace */
    csv = invalid;
  } while (*csv != '\0');
  return count;
}


void SAC4SNetDebugPrintMapping(const char *msg, int *defmap, int cnt)
{
  printf("%s[", msg);
  for (int i = 0; i < cnt; ++i) {
    if (i > 0)
      printf(", ");
    printf("%d", defmap[i]);
  }
  printf("]\n");
}

/************************ Distribution Layer Functions ***********************/
#ifdef ENABLE_DIST_MPI
#include <mpi.h>
#include "pack.h"
static MPI_Datatype SAC4SNetBasetypeToMPIType( int basetype)
{
  switch (basetype) {
    case SACint: return MPI_INT;
    case SACflt: return MPI_FLOAT;
    case SACdbl: return MPI_DOUBLE;
    default:
      Error( "Unsupported basetype in type conversion.");
      break;
  }

  return MPI_DATATYPE_NULL;
}

static void SAC4SNetMPIPackFun(void *sacdata, void *buf)
{
  int *shape;
  void *contents = NULL;
  SACarg *data = sacdata;
  int type, dims, num_elems = 1;

  type = SACARGgetBasetype(data);
  dims = SACARGgetDim(data);
  shape = SNetMemAlloc(dims * sizeof(int));

  for (int i = 0; i < dims; i++) {
    shape[i] = SACARGgetShape(data, i);
    num_elems *= SACARGgetShape(data, i);
  }

  SNetDistribPack(&type, buf, MPI_INT, 1);
  SNetDistribPack(&dims, buf, MPI_INT, 1);
  SNetDistribPack(shape, buf, MPI_INT, dims);

  SNetMemFree(shape);

  switch (type) {
    case SACint:
      contents = SACARGconvertToIntArray(SACARGnewReference(data));
      break;
    case SACdbl:
      contents = SACARGconvertToDoubleArray(SACARGnewReference(data));
      break;
    case SACflt:
      contents = SACARGconvertToFloatArray(SACARGnewReference(data));
      break;
    default:
      Error( "Unsupported basetype in pack function.");
      break;
  }

  SNetDistribPack(contents, buf, SAC4SNetBasetypeToMPIType(type), num_elems);
  SNetMemFree(contents);
}

static void *SAC4SNetMPIUnpackFun(void *buf)
{
  int *shape;
  SACarg *result = NULL;
  void *contents = NULL;
  int type, dims, num_elems = 1;

  SNetDistribUnpack(&type, buf, MPI_INT, 1);
  SNetDistribUnpack(&dims, buf, MPI_INT, 1);
  shape = SNetMemAlloc(dims * sizeof(int));
  SNetDistribUnpack(shape, buf, MPI_INT, dims);

  for (int i = 0; i < dims; i++) {
    num_elems *= shape[i];
  }

  contents = SNetMemAlloc(num_elems * sizeOfType(type));
  SNetDistribUnpack(contents, buf, SAC4SNetBasetypeToMPIType(type), num_elems);

  switch (type) {
    case SACint:
      result = SACARGconvertFromIntPointerVect(contents, dims, shape);
      break;
    case SACflt:
      result = SACARGconvertFromFloatPointerVect(contents, dims, shape);
      break;
    case SACdbl:
      result = SACARGconvertFromDoublePointerVect(contents, dims, shape);
      break;
    default:
      Error( "Unsupported basetype in unpack function.");
      break;
  }

  SNetMemFree(shape);

  return result;
}
#endif

#ifdef ENABLE_DIST_SCC
#include "scc.h"

static void SAC4SNetSCCPackFun(void *cdata, void *buf)
{
}

static void *SAC4SNetSCCUnpackFun(void *buf)
{
  return NULL;
}
#endif
