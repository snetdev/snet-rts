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

#define COUNTS      3
#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

/* Container for returning the result. */
struct container {
  struct handle *hnd;
  snet_variant_t *variant;

  int counter[3];
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
static void *C4SNetAllocFun(size_t, void (**)(void*));
static void C4SNetDataSerialise(FILE *file, c4snet_data_t *ptr);
static c4snet_data_t *C4SNetDataDeserialise(FILE *file);
static void C4SNetDataEncode(FILE *file, c4snet_data_t *ptr);
static c4snet_data_t *C4SNetDataDecode(FILE *file);

/* ID of the language interface. */
static int interface_id;
static void *(*allocfun)(size_t, void (**)(void*)) = &C4SNetAllocFun;


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

size_t C4SNetAllocSize(c4snet_data_t *data)
{ return data->size * C4SNetSizeof(data); }

void *C4SNetAllocFun(size_t s, void (**free)(void*))
{
  *free = &SNetMemFree;
  return SNetMemAlloc(s);
}

/***************************** Common functions ****************************/
#ifdef ENABLE_DIST_MPI
#include <mpi.h>
#include "pack.h"

static void C4SNetMPIPackFun(c4snet_data_t *data, void *buf);
static c4snet_data_t *C4SNetMPIUnpackFun(void *buf);
#endif

#ifdef ENABLE_DIST_SCC
#include "scc.h"
#include "sccmalloc.h"

static void *C4SNetSCCMalloc(size_t, void (**)(void*));
static void C4SNetSCCPackFun(void *cdata, void *dest);
static void *C4SNetSCCUnpackFun(void *localBuf);
#endif


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
        packfun = (void (*)(void*, void*)) &C4SNetMPIPackFun;
        unpackfun = (void *(*)(void*)) &C4SNetMPIUnpackFun;
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
                         (void (*)(void*)) &C4SNetDataFree,
                         (void *(*)(void*)) &C4SNetDataShallowCopy,
                         (size_t (*)(void*)) &C4SNetAllocSize,
                         (void (*)(FILE*, void*)) &C4SNetDataSerialise,
                         (void *(*)(FILE*)) &C4SNetDataDeserialise,
                         (void (*)(FILE*, void*)) &C4SNetDataEncode,
                         (void *(*)(FILE*)) &C4SNetDataDecode,
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
c4snet_data_t *C4SNetDataCreate(c4snet_type_t type, size_t size, const void *data)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->type = type;
  c->ref_count = 1;

  if (size == 1) {
    c->vtype = VTYPE_simple;
    c->size = 0;
    memcpy(&c->data, data, C4SNetSizeof(c));
  } else {
    c->vtype = VTYPE_array;
    c->size = size;
    c->data.ptr = allocfun(C4SNetAllocSize(c), &c->freeFun);
    memcpy(c->data.ptr, data, C4SNetAllocSize(c));
  }

  return c;
}

c4snet_data_t *C4SNetDataAlloc(c4snet_type_t type, int size, void **data)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  c->type = type;
  c->ref_count = 1;

  if (size == 1) {
    c->vtype = VTYPE_simple;
    c->size = 0;
    *data = &c->data;
  } else {
    c->vtype = VTYPE_array;
    c->size = size;
    c->data.ptr = allocfun(C4SNetAllocSize(c), &c->freeFun);
    *data = c->data.ptr;
  }

  return c;
}


/* Increments c4snet_data_t struct's reference count by one.
 * This copy function should be used inside S-Net runtime
 * to avoid needles copying.
 */
c4snet_data_t *C4SNetDataShallowCopy(c4snet_data_t *data)
{
  data->ref_count++;
  return data;
}

/* Copies c4snet_data_t struct.
 * This copy function is available for box code writers.
 *
 */
c4snet_data_t *C4SNetDataCopy(c4snet_data_t *data)
{
  if (data->vtype == VTYPE_array) {
    return C4SNetDataCreate(data->type, data->size, data->data.ptr);
  }

  return C4SNetDataCreate(data->type, data->size, &data->data);
}

/* Frees the memory allocated for c4snet_data_t struct. */
void C4SNetDataFree(c4snet_data_t *data)
{
  if (--data->ref_count == 0) {
    if (data->vtype == VTYPE_array) data->freeFun(data->data.ptr);

    SNetMemFree(data);
  }
}

/* Returns the actual data. */
void *C4SNetDataGetData(c4snet_data_t *data)
{
  if (data->vtype == VTYPE_array) return data->data.ptr;
  return &data->data;
}

int C4SNetDataGetArraySize(c4snet_data_t *data)
{ return data->size; }

static void C4SNetDataSerializeDataPart( FILE *file, c4snet_type_t type, void *data)
{
  switch (type) {
    case CTYPE_uchar:
    case CTYPE_char:
      {
        char c = *(char*) data;
        /* '&' and '<' must be encoded to '&amp;' and 'lt';
        * as they are not legal characters in  XML character data!
        */
        if (c == '<') fprintf(file, "&lt;");
        else if (c == '&') fprintf(file, "&amp;");
        else fprintf(file, "%c", c);
      }
      break;

    case CTYPE_ushort: fprintf(file, "%hu", *(unsigned short *) data); break;
    case CTYPE_short: fprintf(file, "%hd", *(short *) data); break;
    case CTYPE_uint: fprintf(file, "%u", *(unsigned int *) data); break;
    case CTYPE_int: fprintf(file, "%d", *(int *) data); break;
    case CTYPE_ulong: fprintf(file, "%lu", *(unsigned long *) data); break;
    case CTYPE_long: fprintf(file, "%ld", *(long *) data); break;
    case CTYPE_float: fprintf(file, "%.32f", *(float *) data); break;
    case CTYPE_double: fprintf(file, "%.32le", *(double *) data); break;
    case CTYPE_ldouble: fprintf(file, "%.32le", *(long double *) data); break;
    default: SNetUtilDebugFatal("Unknown type in C4SNet.");
  }
}

/* Serializes data to a file using textual representation. */
static void C4SNetDataSerialise( FILE *file, c4snet_data_t *data)
{
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

  fprintf(file, "(%s", type);
  if (data->vtype == VTYPE_array) fprintf(file, "[%d]", data->size);
  fprintf(file, ")");

  if (data->vtype == VTYPE_array) {
    for (int i = 0, size = C4SNetSizeof(data); i < data->size; i++) {
      C4SNetDataSerializeDataPart(file, data->type, (char*) data->data.ptr + i * size);

      if (i < data->size - 1) fprintf(file, ",");
    }
  } else {
    C4SNetDataSerializeDataPart(file, data->type, &data->data);
  }
}

static int C4SNetDataDeserializeTypePart(const char *buf, int size, c4snet_vtype_t *vtype, c4snet_type_t *type)
{
  char *c;
  int count = 0;

  c = strchr(buf, '[');
  if (c == NULL) {
    count = 0;
    *vtype = VTYPE_simple;
  } else {
    count = strtol(c + 1, NULL, 0);
    *vtype = VTYPE_array;
    size = c - buf;
  }


  if (strncmp(buf, "unsigned char", size) == 0)       *type = CTYPE_uchar;
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

static void C4SNetDataDeserializeDataPart(FILE *file, c4snet_type_t type, void *data)
{
  char buf[6];

  char *fmt = NULL;
  switch (type) {
    case CTYPE_char:
    case CTYPE_uchar:
      {
        /* '&' and '<' must be encoded to '&amp;' and 'lt';
        * as they are not legal characters in  XML character data!
        */
        int count = fscanf(file, "%5[^<]", buf);

        if (!strcmp(buf, "&amp;")) *(char *) data = '&';
        else if (!strcmp(buf, "&lt;")) *(char *) data = '<';
        else if (count == 1) *(char *) data = buf[0];
        else SNetUtilDebugFatal("FIXME");

        if (type == CTYPE_uchar) *(unsigned char*) data = (unsigned char) *(char*) data;
        return;
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

  fscanf(file, fmt, data);
}

/* Deserializes textual data from a file. */
static c4snet_data_t *C4SNetDataDeserialise(FILE *file)
{
  c4snet_data_t *temp = SNetMemAlloc(sizeof(c4snet_data_t));
  char buf[32];

  fscanf(file, "(%[^)])", buf);

  temp->size = C4SNetDataDeserializeTypePart(buf, strlen(buf), &temp->vtype, &temp->type);
  temp->ref_count = 1;

  if (temp->vtype == VTYPE_array) {
    temp->data.ptr = allocfun(C4SNetSizeof(temp) * temp->size, &temp->freeFun);
    for (int i = 0; i < temp->size; i++) {
      if (i > 0 && fgetc(file) != ',') SNetUtilDebugFatal("FIXME");

      C4SNetDataDeserializeDataPart(file, temp->type, (char*) temp->data.ptr + i * C4SNetSizeof(temp));
    }
  } else {
    C4SNetDataDeserializeDataPart(file, temp->type, &temp->data);
  }

  return temp;
}

/* Binary encodes data to a file. */
static void C4SNetDataEncode(FILE *file, c4snet_data_t *data)
{
  Base64encodeDataType(file, (int) data->vtype);
  Base64encode(file, &data->size, sizeof(int));
   Base64encodeDataType(file, (int) data->type);

  if (data->vtype == VTYPE_array) {
    Base64encode(file, data->data.ptr, C4SNetAllocSize(data));
  } else {
    Base64encode(file, &data->data, C4SNetSizeof(data));
  }
}

/* Decodes binary data from a file. */
static c4snet_data_t *C4SNetDataDecode(FILE *file)
{
  c4snet_data_t *c = SNetMemAlloc(sizeof(c4snet_data_t));

  Base64decodeDataType(file, (int*) &c->vtype);
  Base64decode(file, &c->size, sizeof(int));
  Base64decodeDataType(file, (int*) &c->type);

  c->ref_count = 1;
  c->freeFun = &SNetMemFree;

  if(c->vtype == VTYPE_array) {
    c->data.ptr = allocfun(C4SNetAllocSize(c), &c->freeFun);
    Base64decode(file, c->data.ptr, C4SNetAllocSize(c));
  } else {
    Base64decode(file, &c->data, C4SNetSizeof(c));
  }

  return c;
}

/**************************** Container functions ***************************/

/* Creates a container that can be used to return values. */
c4snet_container_t *C4SNetContainerCreate(void *hnd, int variant)
{
  c4snet_container_t *c = SNetMemAlloc(sizeof(c4snet_container_t));

  //variant - 1: Lists are zero indexed, but existing S-Net code defines
  //variants as 1-indexed. Go backwards compatibility!
  c->variant = SNetVariantListGet(SNetHndGetVariantList(hnd), variant - 1);

  c->fields = SNetMemAlloc(SNetVariantNumFields(c->variant) * sizeof(void*));
  c->tags = SNetMemAlloc(SNetVariantNumTags(c->variant) * sizeof(int));
  c->btags = SNetMemAlloc(SNetVariantNumBTags(c->variant) * sizeof(int));
  c->hnd = hnd;

  for (int i = 0; i < COUNTS; i++) c->counter[i] = 0;

  return c;
}

/* Adds field into a container. */
void C4SNetContainerSetField(c4snet_container_t *c, void *ptr)
{ c->fields[F_COUNT(c)++] = ptr; }

/* Adds tag into a container. */
void C4SNetContainerSetTag(c4snet_container_t *c, int value)
{ c->tags[T_COUNT(c)++] = value; }

/* Adds binding tag into a container. */
void C4SNetContainerSetBTag(c4snet_container_t *c, int value)
{ c->btags[B_COUNT(c)++] = value; }

/* Communicates back the results of the box stored in a container. */
void C4SNetContainerOut(c4snet_container_t *c)
{
  SNetOutRawArray(c->hnd, interface_id, c->variant, c->fields, c->tags, c->btags);
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
    default: SNetUtilDebugFatal("Unknown MPI datatype!\n");
  }

  return MPI_CHAR;
}

static void C4SNetMPIPackFun(c4snet_data_t *data, void *buf)
{
  int vtype = data->vtype,
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

static c4snet_data_t *C4SNetMPIUnpackFun(void *buf)
{
  void *tmp;
  c4snet_data_t *result;
  int vtype, type, count;

  SNetDistribUnpack(&vtype, buf, MPI_INT, 1);
  SNetDistribUnpack(&count, buf, MPI_INT, 1);
  SNetDistribUnpack(&type, buf, MPI_INT, 1);

  if (vtype == VTYPE_array) {
    result = C4SNetDataAlloc(type, count, &tmp);
    SNetDistribUnpack(tmp, buf, C4SNetTypeToMPIType(type), count);
  } else {
    result = C4SNetDataCreate(type, 1, &tmp);
    SNetDistribUnpack(tmp, buf, C4SNetTypeToMPIType(type), 1);
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

static void C4SNetSCCPackFun(c4snet_data_t *cdata, void *buf)
{
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
