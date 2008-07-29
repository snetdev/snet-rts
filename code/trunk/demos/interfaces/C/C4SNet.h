#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stdio.h>

/* C data structure. Can only contain primary type. */
typedef struct cdata c4snet_data_t;

/* Container for data*/
typedef struct container c4snet_container_t;

/* Type enumeration for C primary types. */
typedef enum{
    CTYPE_unknown,  /* Unkown data type. Used for error messages, not a valid type! */
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
}c4snet_type_t;


/***************************** Common functions ****************************/
void C4SNetInit( int id);
void C4SNetOut( void *hnd, int variant, ...);

/****************************** Data functions *****************************/

c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, void *data);
void *C4SNetDataCopy( void *ptr);
void C4SNetDataFree( void *ptr);

void *C4SNetDataGetData( c4snet_data_t *c);
c4snet_type_t C4SNetDataGetType( c4snet_data_t *c);

int C4SNetDataSerialize( FILE *file, void *ptr);
void *C4SNetDataDeserialize(FILE *file);

int C4SNetDataEncode( FILE *file, void *ptr);
void *C4SNetDataDecode(FILE *file);

/**************************** Container functions ***************************/
/* TODO: Would 'record' be better than 'container'? */

c4snet_container_t *C4SNetContainerCreate( void *hnd, int var_num);

c4snet_container_t *C4SNetContainerSetField( c4snet_container_t *c, void *ptr);
c4snet_container_t *C4SNetContainerSetTag( c4snet_container_t *c, int val);
c4snet_container_t *C4SNetContainerSetBTag( c4snet_container_t *c, int val);

void C4SNetContainerOut( c4snet_container_t *c);

#endif /* _C4SNET_H_ */
