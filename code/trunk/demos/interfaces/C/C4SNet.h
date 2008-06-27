#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stdio.h>

typedef struct container c4snet_container_t;

/* Type enumeration for C primary types. */
typedef enum{
  CTYPE_unknown,    /* Unkown data type. Used for error messages, not a valid type! */
    CTYPE_uchar,    /* unsigned char      */
    CTYPE_char,     /* signed char        */
    CTYPE_ushort,   /* unsigned short int */
    CTYPE_short,    /* signed short int   */ 
    CTYPE_uint,     /* unsigned int       */ 
    CTYPE_int,      /* signed int         */ 
    CTYPE_ulong,    /* unsigned long int  */
    CTYPE_long,     /* signed long int    */ 
    CTYPE_float,    /* float              */ 
    CTYPE_double,   /* double             */ 
    CTYPE_ldouble,  /* long double        */ 
}ctype_t;

/* Union of all the types to be used as generic data type. */
typedef union cdata_types{
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
} cdata_types_t;

/* C data structure. Can only contain primary type. */
typedef struct cdata {
  ctype_t type;
  cdata_types_t data;
} C_Data;

void C4SNetFree( void *ptr);
void *C4SNetCopy( void *ptr);

int C4SNetEncode( FILE *file, void *ptr);
void *C4SNetDecode(FILE *file);

void C4SNetInit( int id);

void C4SNet_outCompound( c4snet_container_t *c);
void C4SNet_out( void *hnd, int variant, ...);

/* ************************************************************************* */

/* C_Data */

C_Data *C4SNet_cdataCreate( ctype_t type, void *data);

void *C4SNet_cdataGetData( C_Data *c);

ctype_t C4SNet_cdataGetType( C_Data *c);

/* ************************************************************************* */

c4snet_container_t *C4SNet_containerCreate( void *hnd, int var_num);
c4snet_container_t *C4SNet_containerSetField( c4snet_container_t *c, void *ptr);
c4snet_container_t *C4SNet_containerSetTag( c4snet_container_t *c, int val);
c4snet_container_t *C4SNet_containerSetBTag( c4snet_container_t *c, int val);
#endif 
