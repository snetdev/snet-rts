#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stddef.h>
#include "type.h"

typedef struct cdata c4snet_data_t;
typedef struct container c4snet_container_t;

typedef enum{
    VTYPE_simple,  /*!< simple type */
    VTYPE_array    /*!< array  type */
}c4snet_vtype_t;

typedef enum{
    CTYPE_uchar,   /*!< unsigned char */
    CTYPE_char,    /*!< signed char */
    CTYPE_ushort,  /*!< unsigned short int */
    CTYPE_short,   /*!< signed short int */
    CTYPE_uint,    /*!< unsigned int */
    CTYPE_int,     /*!< signed int */
    CTYPE_ulong,   /*!< unsigned long int */
    CTYPE_long,    /*!< signed long int */
    CTYPE_float,   /*!< float */
    CTYPE_double,  /*!< double */
    CTYPE_ldouble, /*!< long double */
    CTYPE_string   /* Experimental type for supporting strings */
}c4snet_type_t;

void C4SNetInit( int id, snet_distrib_t distImpl);
void C4SNetOut( void *hnd, int variant, ...);

int C4SNetSizeof(c4snet_data_t *data);

c4snet_data_t *C4SNetDataCreate(c4snet_type_t type, size_t size, const void *data);
c4snet_data_t *C4SNetDataAlloc(c4snet_type_t type, int size, void **data);
c4snet_data_t *C4SNetDataCopy(c4snet_data_t *ptr);
c4snet_data_t *C4SNetDataShallowCopy(c4snet_data_t *ptr);
void C4SNetDataFree(c4snet_data_t *ptr);
void *C4SNetDataGetData( c4snet_data_t *ptr);
int C4SNetDataGetArraySize( c4snet_data_t *c);

c4snet_container_t *C4SNetContainerCreate( void *hnd, int variant);
void C4SNetContainerSetField(c4snet_container_t *c, void *ptr);
void C4SNetContainerSetTag(c4snet_container_t *c, int value);
void C4SNetContainerSetBTag(c4snet_container_t *c, int value);
void C4SNetContainerOut( c4snet_container_t *c);
#endif /* _C4SNET_H_ */
