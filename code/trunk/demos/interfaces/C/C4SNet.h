#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stdio.h>
#include "C4SNetTypes.h"

typedef struct container c4snet_container_t;

typedef struct cdata {
 void *data;
 void (*freefun)( void*);
 void* (*copyfun)( void*);
 int (*serfun)(FILE *, void*);
 int (*encfun)(FILE *, void*);
} C_Data;


void C4SNetInit( int id);

void C4SNet_outCompound( c4snet_container_t *c);
void C4SNet_out( void *hnd, int variant, ...);

void C4SNet_free( void *ptr);
void *C4SNet_copy( void *ptr);
int C4SNet_serialize( FILE *file, void *ptr);
void *C4SNet_deserialize( FILE *file);
int C4SNet_encode( FILE *file, void *ptr);
void *C4SNet_decode( FILE *file);

/* ************************************************************************* */

/* C_Data */

C_Data *C4SNet_cdataCreate( void *data, 
		     	    void (*freefun)( void*),
		     	    void* (*copyfun)( void*),
			    int (*serfun)( FILE *, void*),
			    int (*encfun)( FILE *, void*));

void *C4SNet_cdataGetData( C_Data *c);
void *C4SNet_cdataGetCopyFun( C_Data *c);
void *C4SNet_cdataGetFreeFun( C_Data *c);
void *C4SNet_cdataGetSerFun( C_Data *c);
void *C4SNet_cdataGetEncFun( C_Data *c);

/* ************************************************************************* */

c4snet_container_t *C4SNet_containerCreate( void *hnd, int var_num);
c4snet_container_t *C4SNet_containerSetField( c4snet_container_t *c, void *ptr);
c4snet_container_t *C4SNet_containerSetTag( c4snet_container_t *c, int val);
c4snet_container_t *C4SNet_containerSetBTag( c4snet_container_t *c, int val);
#endif 
