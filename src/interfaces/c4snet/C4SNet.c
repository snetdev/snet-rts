/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file C4SNet.c
 *
 * C4SNet is a simple C language interface for S-Net.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "debug.h"
#include "C4SNet.h"
#include "memfun.h"
#include "snettypes.h"
#include "type.h"
#include "interface_functions.h"
#include "out.h"
#include "base64.h"

#ifdef ENABLE_DIST_MPI
#include <mpi.h>
#include "pack.h"

static void C4SNetMPIPackFun(void *cdata, void *buf);
static void *C4SNetMPIUnpackFun(void *buf);
#endif

#ifdef ENABLE_DIST_SCC
#include "scc.h"
#include "sccmalloc.h"

static void *C4SNetSCCMalloc(size_t, void (**)(void*));
static void C4SNetSCCPackFun(void *cdata, void *dest);
static void *C4SNetSCCUnpackFun(void *localBuf);
#endif

#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

#define BUF_SIZE 32

static void *C4SNetAllocFun(size_t, void (**)(void*));

/* ID of the language interface. */
static int interface_id;
static void *(*allocfun)(size_t, void (**)(void*)) = &C4SNetAllocFun;

/* Container for returning the result. */
struct container {
  struct handle *hnd;
  snet_variant_t *variant;

  int *counter;
  void **fields;
  int *tags;
  int *btags;
};

/* Union containing all C primary types. */
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
  char *str;                  /* Experimental for string */
  void *ptr;
} c4snet_primary_type_t;

/* Data struct to store the actual data. */
struct cdata {
  unsigned int ref_count;     /* reference count for garbage collection. */
  c4snet_vtype_t vtype;       /* Variable type. */
  int size;                   /* Number of elements in the array. */
  void (*freeFun)(void*);
  c4snet_type_t type;         /* C type of the data. */
  c4snet_primary_type_t data; /* The data. */
};

/* Interface functions that are needed by the runtime environment,
 * but are not given to the box programmer.
 */
static int C4SNetDataSerialize( FILE *file, void *ptr);
static void *C4SNetDataDeserialize(FILE *file);
static int C4SNetDataEncode( FILE *file, void *ptr);
static void *C4SNetDataDecode(FILE *file);

/***************************** Auxiliary functions ****************************/

static int sizeOfType(c4snet_type_t type)
{
  switch (type) {
    case CTYPE_uchar: return sizeof(unsigned char);
    case CTYPE_char: return sizeof(signed char);
    case CTYPE_ushort: return sizeof(unsigned short);
    case CTYPE_short: return sizeof(signed short);
    case CTYPE_uint: return sizeof(unsigned int);
    case CTYPE_int: return sizeof(signed int);
    case CTYPE_ulong: return sizeof(unsigned long);
    case CTYPE_long: return sizeof(signed long);
    case CTYPE_float: return sizeof(float);
    case CTYPE_double: return sizeof(double);
    case CTYPE_ldouble: return sizeof(long double);
    case CTYPE_string: return sizeof(char *);
    default:
      SNetUtilDebugFatal("Unknown type in C4SNet language interface!");
  }

  return 0;
}

/* Return size of the data type. */
int C4SNetSizeof(c4snet_data_t *data)
{ return sizeOfType(data->type); }

size_t C4SNetAllocSize(void *cdata)
{
  c4snet_data_t *data = cdata;
  if (data->vtype == VTYPE_simple) return 0;

  return data->size * C4SNetSizeof(data);
}

void *C4SNetAllocFun(size_t s, void (**free)(void*))
{
  *free = &SNetMemFree;
  return SNetMemAlloc(s);
}

/***************************** Common functions ****************************/

/* Language interface initialization function. */
void C4SNetInit( int id, snet_distrib_t distImpl)
{
  interface_id = id;
  snet_pack_fun_t packfun = NULL;
  snet_unpack_fun_t unpackfun = NULL;

  switch (distImpl) {
    case nodist:
      break;
    case mpi:
      #ifdef ENABLE_DIST_MPI
        packfun = &C4SNetMPIPackFun;
        unpackfun = &C4SNetMPIUnpackFun;
      #else
        SNetUtilDebugFatal("C4SNet supports MPI, but is not configured to use "
                           "it.\n");
      #endif
      break;
    case scc:
      #ifdef ENABLE_DIST_SCC
        allocfun = &C4SNetSCCMalloc;
        packfun = &C4SNetSCCPackFun;
        unpackfun = &C4SNetSCCUnpackFun;
      #else
        SNetUtilDebugFatal("C4SNet supports SCC, but is not configured to use "
                           "it.\n");
      #endif
      break;
    default:
      SNetUtilDebugFatal("C4SNet doesn't support the selected distribution "
                         "layer (%d).\n", distImpl);
      break;
  }

  SNetInterfaceRegister( id,
                         &C4SNetDataFree,
                         &C4SNetDataShallowCopy,
                         &C4SNetAllocSize,
                         &C4SNetDataSerialize,
                         &C4SNetDataDeserialize,
                         &C4SNetDataEncode,
                         &C4SNetDataDecode,
                         packfun,
                         unpackfun);
}

/* Communicates back the results of the box. */
void C4SNetOut( void *hnd, int variant, ...)
{
  va_list args;
  va_start(args, variant);
  SNetOutRawV(hnd, interface_id, variant, args);
  va_end(args);
}

/****************************** Data functions *****************************/

/* Creates a new c4snet_data_t struct. */
c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, const void *data)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->vtype = VTYPE_simple;
  c->size = -1;
  c->type = type;
  c->ref_count = 1;
  c->freeFun = &SNetMemFree;

  if (type == CTYPE_string) {
    c->data.str = strdup((char*) data);
  } else {
    memcpy((void*) &c->data, data, C4SNetSizeof(c));
  }

  return c;
}

c4snet_data_t *C4SNetDataCreateArray( c4snet_type_t type, int size, void *data)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->vtype = VTYPE_array;
  c->size = size;
  c->type = type;
  c->ref_count = 1;
  c->freeFun = &SNetMemFree;
  c->data.ptr = data;

  return c;
}

c4snet_data_t *C4SNetDataAllocCreateArray( c4snet_type_t type, int size, void **data)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->vtype = VTYPE_array;
  c->size = size;
  c->type = type;
  c->ref_count = 1;
  c->data.ptr = *data = allocfun(size * sizeOfType(type), &c->freeFun);

  return c;
}

/* Increments c4snet_data_t struct's reference count by one.
 * This copy function should be used inside S-Net runtime
 * to avoid needles copying.
 */
void *C4SNetDataShallowCopy(void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t*) ptr;
  temp->ref_count++;
  return ptr;
}

/* Copies c4snet_data_t struct.
 * This copy function is available for box code writers.
 *
 */
void *C4SNetDataCopy( void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->vtype = temp->vtype;
  c->size = temp->size;
  c->type = temp->type;
  c->freeFun = &SNetMemFree;
  c->ref_count = 1;

  if (c->vtype == VTYPE_array) {
    int size = C4SNetAllocSize(c);
    c->data.ptr = SNetMemAlloc(size);
    memcpy(c->data.ptr, temp->data.ptr, size);
  } else if (c->type == CTYPE_string) {
    c->data.str = strdup(temp->data.str);
  } else {
    memcpy(&c->data, &temp->data, C4SNetSizeof(c));
  }

  return c;
}

/* Frees the memory allocated for c4snet_data_t struct. */
void C4SNetDataFree( void *ptr)
{
  c4snet_data_t *data = ptr;

  data->ref_count--;

  if (data->ref_count == 0) {
    if (data->vtype == VTYPE_array) {
      if (data->freeFun) data->freeFun(data->data.ptr);
      else SNetMemFree(data->data.ptr);
    }

    SNetMemFree(ptr);
  }
}

/* Returns the actual data. */
void *C4SNetDataGetData( c4snet_data_t *ptr)
{
  if(ptr->type == CTYPE_string) return ptr->data.str;
  if(ptr->vtype == VTYPE_array) return ptr->data.ptr;
  return &ptr->data;
}

int C4SNetDataGetArraySize( c4snet_data_t *ptr)
{
  if (ptr->vtype == VTYPE_array) return ptr->size;
  return -1;
}

static int C4SNetDataSerializeDataPart( FILE *file, c4snet_type_t type, void *data)
{
  char c;
  switch (type) {
    case CTYPE_uchar:
    case CTYPE_char:
      c = *(char*)data;
      /* '&' and '<' must be encoded to '&amp;' and 'lt';
       * as they are not legal characters in  XML character data!
       */
      if (c == '<') return fprintf(file, "&lt;");
      else if (c == '&') return fprintf(file, "&amp;");
      else return fprintf(file, "%c", c);

    case CTYPE_string:
      return fprintf(file, "\"%s\"", ((c4snet_primary_type_t*) data)->str);

    case CTYPE_ushort: return fprintf(file, "%hu", *(unsigned short *)data);
    case CTYPE_short: return fprintf(file, "%hd", *(short *)data);
    case CTYPE_uint: return fprintf(file, "%u", *(unsigned int *)data);
    case CTYPE_int: return fprintf(file, "%d", *(int *)data);
    case CTYPE_ulong: return fprintf(file, "%lu", *(unsigned long *)data);
    case CTYPE_long: return fprintf(file, "%ld", *(long *)data);
    case CTYPE_float: return fprintf(file, "%.32f", *(float *)data);
    case CTYPE_double: return fprintf(file, "%.32le", *(double *)data);
    case CTYPE_ldouble: return fprintf(file, "%.32le", *(double *)data);
    default: SNetUtilDebugFatal("Unknown type in C4SNet.");
  }

  return 0;
}

/* Serializes data to a file using textual representation. */
static int C4SNetDataSerialize( FILE *file, void *ptr)
{
  int ret = 0;
  c4snet_data_t *data = (c4snet_data_t*) ptr;

  char *type = NULL;
  switch (data->type) {
    case CTYPE_uchar: type = "unsigned char"; break;
    case CTYPE_char: type = "char"; break;
    case CTYPE_ushort: type = "unsigned short"; break;
    case CTYPE_short: type = "short"; break;
    case CTYPE_uint: type = "unsigned int"; break;
    case CTYPE_int: type = "int"; break;
    case CTYPE_ulong: type = "unsigned long"; break;
    case CTYPE_long: type = "long"; break;
    case CTYPE_float: type = "float"; break;
    case CTYPE_double: type = "double"; break;
    case CTYPE_ldouble: type = "long double"; break;
    case CTYPE_string: type = "string"; break;
    default: SNetUtilDebugFatal("Unknown type in C4SNet.");
  }

  ret += fprintf(file, "(%s", type);

  if (data->vtype == VTYPE_array) ret += fprintf(file, "[%d]", data->size);

  ret += fprintf(file, ")");
  if (data->vtype == VTYPE_array) {
    int size = C4SNetSizeof(data);

    for (int i = 0; i < data->size; i++) {
      // TODO This conversion `(c4snet_primary_type_t *)((size_t)data->data.ptr + i * size)' 
      // should be chekced properly, this is done for compiling this file with g++
      ret += C4SNetDataSerializeDataPart(file, data->type, (char*) data->data.ptr + i * size);

      if (i < data->size - 1) ret += fprintf(file, ",");
    }
  } else {
    ret += C4SNetDataSerializeDataPart(file, data->type, &data->data);
  }

  return ret;
}

static int C4SNetDataDeserializeTypePart(const char *buf, int size, c4snet_vtype_t *vtype, c4snet_type_t *type)
{
  char *c;
  int count = 0;

  c = strchr(buf, '[');
  if (c == NULL) {
    count = -1;
    *vtype = VTYPE_simple;
  } else {
    count = strtol(c + 1, NULL, 0);
    *vtype = VTYPE_array;
    size = c - buf;
  }


  if (strncmp(buf, "unsigned char", size) == 0)        *type = CTYPE_uchar;
  else if(strncmp(buf, "char", size) == 0)            *type = CTYPE_char;
  else if(strncmp(buf, "unsigned short", size) == 0)  *type = CTYPE_ushort;
  else if(strncmp(buf, "short", size) == 0)           *type = CTYPE_short;
  else if(strncmp(buf, "unsigned int", size) == 0)    *type = CTYPE_uint;
  else if(strncmp(buf, "int", size) == 0)             *type = CTYPE_int;
  else if(strncmp(buf, "unsigned long", size) == 0)   *type = CTYPE_ulong;
  else if(strncmp(buf, "long", size) == 0)            *type = CTYPE_long;
  else if(strncmp(buf, "float", size) == 0)           *type = CTYPE_float;
  else if(strncmp(buf, "double", size) == 0)          *type = CTYPE_double;
  else if(strncmp(buf, "long double", size) == 0)     *type = CTYPE_ldouble;
  else if(strncmp(buf, "string", size) == 0)          *type = CTYPE_string;
  else SNetUtilDebugFatal("C4SNet interface encountered an unknown type.");

  return count;
}

static int C4SNetDataDeserializeDataPart( FILE *file, c4snet_type_t type, void *data)
{
  char buf[5];

  char *fmt = NULL;
  switch (type) {
    case CTYPE_uchar:
      /* '&' and '<' must be encoded to '&amp;' and 'lt';
       * as they are not legal characters in  XML character data!
       */
      if (fscanf(file, "%c", (char*) data) != 1) return 0;

      if (*(char*) data == '&') {
        if (fgets(buf, 5, file) == NULL) SNetUtilDebugFatal("FIXME");

        if (strcmp(buf, "amp;") == 0) *(char*) data = '&';
        else if (strcmp(buf, "lt;") == 0) *(char*) data = '<';
        else SNetUtilDebugFatal("FIXME");
      } else if (*(char*) data == '<') {
        SNetUtilDebugFatal("FIXME");
      }

      *(unsigned char*) data = (unsigned char) *(char*)data;
      return 1;

    case CTYPE_char:
      /* '&' and '<' must be encoded to '&amp;' and 'lt';
       * as they are not legal characters in  XML character data!
       */
      if (fscanf(file, "%c", (char*) data) != 1) return 0;

      if (*(char*) data == '&') {
        if (fgets(buf, 5, file) == NULL) SNetUtilDebugFatal("FIXME");

        if (strcmp(buf, "amp;") == 0) *(char*) data = '&';
        else if (strcmp(buf, "lt;") == 0) *(char*) data = '<';
        else SNetUtilDebugFatal("FIXME");
      } else if (*(char*) data == '<') {
        SNetUtilDebugFatal("FIXME");
      }
      return 1;

    case CTYPE_string:
      {
#   define SIZE 3
        char * buffer=NULL;
        int size=0, cur=0;
        char symbol;
        int res;
        res = fscanf(file, "%c", &symbol);
        assert(res != EOF || errno == 0);

        if (symbol != '\"') return 0;
        while (symbol != 0) {
          res = fscanf(file, "%c", &symbol);
          assert(res != EOF || errno == 0);
          if (symbol == '\"') symbol = 0;
          if (cur == size) {
            size += SIZE;
            if (buffer == NULL) {
              buffer = (char *) malloc(size * sizeof(char));
            }
            else
              buffer = (char *) realloc(buffer, size * sizeof(char));
          }
          buffer[cur++] = symbol;
        }
        *(char**) data = buffer;
        return 1;
      }

    case CTYPE_ushort: fmt = "%hu"; break;
    case CTYPE_short: fmt = "%hd"; break;
    case CTYPE_uint: fmt = "%u"; break;
    case CTYPE_int: fmt = "%d"; break;
    case CTYPE_ulong: fmt = "%lu"; break;
    case CTYPE_long: fmt = "%ld"; break;
    case CTYPE_float: fmt = "%f"; break;
    case CTYPE_double: fmt = "%lf"; break;
    case CTYPE_ldouble: fmt = "%Lf"; break;
    default: SNetUtilDebugFatal("FIXME");
  }

  return fscanf(file, fmt, data);
}

/* Deserializes textual data from a file. */
static void *C4SNetDataDeserialize(FILE *file)
{
  c4snet_data_t *temp = NULL;
  char buf[BUF_SIZE];
  int j;
  char c;
  int size;
  int i;

  fscanf(file, "(%[^)])", buf);
  j = strlen(buf);

  temp = SNetMemAlloc(sizeof(c4snet_data_t));

  temp->size = C4SNetDataDeserializeTypePart(buf, j, &temp->vtype, &temp->type);

  temp->ref_count = 1;
  temp->freeFun = &SNetMemFree;

  if (temp->vtype == VTYPE_array) {
    size = C4SNetSizeof(temp);

    temp->data.ptr = SNetMemAlloc(size * temp->size);
    for (i = 0; i < temp->size; i++) {
      if (i > 0 && (c = fgetc(file)) != ',') {
        SNetUtilDebugFatal("FIXME");
      }

      if (!C4SNetDataDeserializeDataPart(file, temp->type, (char*) temp->data.ptr + i * size)) {
        SNetUtilDebugFatal("FIXME");
      }
    }
  } else {
    if (!C4SNetDataDeserializeDataPart(file, temp->type, &temp->data)) {
      SNetUtilDebugFatal("FIXME");
    }
  }

  return temp;
}

/* Binary encodes data to a file. */
static int C4SNetDataEncode( FILE *file, void *ptr)
{
  int i = 0;
  c4snet_data_t *temp = ptr;

  Base64encodeDataType(file, (int)temp->vtype);
  Base64encode(file, &temp->size, sizeof(int));
  i = Base64encodeDataType(file, (int)temp->type);

  if(temp->vtype == VTYPE_array) {
    i += Base64encode(file, temp->data.ptr, C4SNetSizeof(temp) * temp->size);
  } else {
    i += Base64encode(file, &temp->data, C4SNetSizeof(temp));
  }

  return i + 1;
}

/* Decodes binary data from a file. */
static void *C4SNetDataDecode(FILE *file)
{
  c4snet_data_t *c = SNetMemAlloc( sizeof( c4snet_data_t));
  int i = 0;
  int size;

  Base64decodeDataType(file, (int *)&c->vtype);
  Base64decode(file, (unsigned char *)&c->size, sizeof(int));
  i = Base64decodeDataType(file, (int *)&c->type);

  c->ref_count = 1;
  c->freeFun = &SNetMemFree;

  size = C4SNetSizeof(c);

  if(c->vtype == VTYPE_array) {
    c->data.ptr = SNetMemAlloc(size * c->size);
    i += Base64decode(file, c->data.ptr, size * c->size);
  } else {
    i += Base64decode(file, &c->data, size);
  }

  return c;
}



/**************************** Container functions ***************************/

/* Creates a container that can be used to return values. */
c4snet_container_t *C4SNetContainerCreate( void *hnd, int variant)
{
  c4snet_container_t *c = SNetMemAlloc( sizeof( c4snet_container_t));

  //variant - 1: Lists are zero indexed, but existing S-Net code defines
  //variants as 1-indexed. Go backwards compatibility!
  c->variant = SNetVariantListGet( SNetHndGetVariantList((struct handle *) hnd), variant - 1);

  c->counter = SNetMemAlloc( 3 * sizeof( int));

  c->fields = SNetMemAlloc( SNetVariantNumFields( c->variant) * sizeof(void*));
  c->tags = SNetMemAlloc( SNetVariantNumTags( c->variant) * sizeof( int));
  c->btags = SNetMemAlloc( SNetVariantNumBTags( c->variant) * sizeof( int));
  c->hnd = hnd;

  for (int i = 0; i < 3; i++) {
    c->counter[i] = 0;
  }

  return c;
}


/* Adds field into a container. */
c4snet_container_t *C4SNetContainerSetField(c4snet_container_t *c, void *ptr)
{
  c->fields[F_COUNT(c)++] = ptr;
  return c;
}

/* Adds tag into a container. */
c4snet_container_t *C4SNetContainerSetTag(c4snet_container_t *c, int value)
{
  c->tags[T_COUNT(c)++] = value;
  return c;
}

/* Adds binding tag into a container. */
c4snet_container_t *C4SNetContainerSetBTag(c4snet_container_t *c, int value)
{
  c->btags[B_COUNT(c)++] = value;
  return c;
}

/* Communicates back the results of the box stored in a container. */
void C4SNetContainerOut(c4snet_container_t *c)
{
  SNetOutRawArray( c->hnd, interface_id, c->variant, c->fields, c->tags, c->btags);

  SNetMemFree(c->counter);
  SNetMemFree(c);
}

/************************ Distribution Layer Functions ***********************/
#ifdef ENABLE_DIST_MPI
static MPI_Datatype C4SNetTypeToMPIType(c4snet_type_t type)
{
  switch (type) {
    case CTYPE_uchar: return MPI_UNSIGNED_CHAR;
    case CTYPE_char: return MPI_CHAR;
    case CTYPE_ushort: return MPI_UNSIGNED_SHORT;
    case CTYPE_short: return MPI_SHORT;
    case CTYPE_uint: return MPI_UNSIGNED;
    case CTYPE_int: return MPI_INT;
    case CTYPE_ulong: return MPI_UNSIGNED_LONG;
    case CTYPE_long: return MPI_LONG;
    case CTYPE_float: return MPI_FLOAT;
    case CTYPE_double: return MPI_DOUBLE;
    case CTYPE_ldouble: return MPI_LONG_DOUBLE;
    default:
      break;
  }

  SNetUtilDebugFatal("Unknown MPI datatype!\n");
  return MPI_CHAR;
}

static void C4SNetMPIPackFun(void *cdata, void *buf)
{
  int vtype, type;
  c4snet_data_t *data = (c4snet_data_t*) cdata;

  vtype = data->vtype;
  type = data->type;

  SNetDistribPack(&vtype, buf, MPI_INT, 1);
  SNetDistribPack(&data->size, buf, MPI_INT, 1);
  SNetDistribPack(&type, buf, MPI_INT, 1);

  if (data->vtype == VTYPE_array) {
    SNetDistribPack(data->data.ptr, buf, C4SNetTypeToMPIType(data->type),
                    data->size);
  } else {
    SNetDistribPack(&data->data, buf, C4SNetTypeToMPIType(data->type), 1);
  }
}

static void *C4SNetMPIUnpackFun(void *buf)
{
  c4snet_data_t *result;
  int vtype, type, count;
  c4snet_primary_type_t data;

  SNetDistribUnpack(&vtype, buf, MPI_INT, 1);
  SNetDistribUnpack(&count, buf, MPI_INT, 1);
  SNetDistribUnpack(&type, buf, MPI_INT, 1);

  if (vtype == VTYPE_array) {
    data.ptr = SNetMemAlloc(count * sizeOfType(type));
    SNetDistribUnpack(data.ptr, buf, C4SNetTypeToMPIType(type), count);
    result = C4SNetDataCreateArray(type, count, data.ptr);
  } else {
    SNetDistribUnpack(&data, buf, C4SNetTypeToMPIType(type), 1);
    result = C4SNetDataCreate(type, &data);
  }

  return result;
}
#endif

#ifdef ENABLE_DIST_SCC
static void *C4SNetSCCMalloc(size_t s, void (**free)(void*))
{
  if (!remap) return C4SNetAllocFun(s, free);

  *free = &SCCFree;
  return SCCMallocPtr(s);
}

static void C4SNetSCCPackFun(void *cdata, void *buf)
{
  c4snet_data_t *data = cdata;
  SNetDistribPack(cdata, buf, sizeof(c4snet_data_t), false);

  if (data->vtype == VTYPE_array) {
    if (remap && data->freeFun == &SNetMemFree) {
      void *tmp = SCCMallocPtr(C4SNetAllocSize(cdata));
      memcpy(tmp, data->data.ptr, C4SNetAllocSize(cdata));
      SNetMemFree(data->data.ptr);
      data->freeFun = &SCCFree;
      data->data.ptr = tmp;
    }

    SNetDistribPack(data->data.ptr, buf, C4SNetAllocSize(data), true);
  }
}

static void *C4SNetSCCUnpackFun(void *buf)
{
  c4snet_data_t *result = SNetMemAlloc(sizeof(c4snet_data_t));
  SNetDistribUnpack(result, buf, false, sizeof(c4snet_data_t));

  result->ref_count = 1;
  if (result->vtype == VTYPE_array) {
    result->freeFun = &SCCFree;
    SNetDistribUnpack(&result->data.ptr, buf, true);
  }

  return result;
}
#endif
