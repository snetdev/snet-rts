#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stddef.h>
#include "type.h"

typedef struct cdata c4snet_data_t;
typedef struct container c4snet_container_t;

typedef enum {
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
} c4snet_type_t;

void C4SNetOut(void *hnd, int variant, ...);

int C4SNetSizeof(c4snet_data_t *data);
size_t C4SNetArraySize(c4snet_data_t *data);

c4snet_data_t *C4SNetAlloc(c4snet_type_t type, size_t size, void **data);
c4snet_data_t *C4SNetCreate(c4snet_type_t type, size_t size, const void *data);
void *C4SNetGetData(c4snet_data_t *ptr);
void C4SNetFree(c4snet_data_t *ptr);

c4snet_data_t *C4SNetShallowCopy(c4snet_data_t *ptr);
c4snet_data_t *C4SNetDeepCopy(c4snet_data_t *ptr);

/**************************** Container functions ****************************/

c4snet_container_t *C4SNetContainerCreate(void *hnd, int variant);
void C4SNetContainerSetField(c4snet_container_t *c, void *ptr);
void C4SNetContainerSetTag(c4snet_container_t *c, int value);
void C4SNetContainerSetBTag(c4snet_container_t *c, int value);
void C4SNetContainerOut(c4snet_container_t *c);
#endif /* _C4SNET_H_ */
