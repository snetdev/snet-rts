#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stdio.h>
#include "C4SNetTypes.h"

typedef struct container c4snet_container_t;

typedef enum{
  CTYPE_uchar,      //char
    CTYPE_char,     //signed char
    CTYPE_ushort,   //unsigned short
    CTYPE_short,    //short
    CTYPE_uint,     //unsigned int
    CTYPE_int,      //int,
    CTYPE_ulong,    //unsigned long
    CTYPE_long,     //long
    CTYPE_float,    //float
    CTYPE_double,   //double
}ctype_t;

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
} cdata_types_t;

typedef struct cdata {
  ctype_t type;
  cdata_types_t data;
} C_Data;


void C4SNetInit( int id);

void C4SNet_outCompound( c4snet_container_t *c);
void C4SNet_out( void *hnd, int variant, ...);

void C4SNetFree( void *ptr);
void *C4SNetCopy( void *ptr);
int C4SNetSerialize( FILE *file, void *ptr);
void *C4SNetDeserialize( FILE *file);
int C4SNetEncode( FILE *file, void *ptr);
void *C4SNetDecode( FILE *file);

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
