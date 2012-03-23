#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stddef.h>
#include "type.h"

typedef struct cdata c4snet_data_t;

typedef enum {
    CTYPE_uchar,   /* unsigned char */
    CTYPE_char,    /* signed char */
    CTYPE_ushort,  /* unsigned short int */
    CTYPE_short,   /* signed short int */
    CTYPE_uint,    /* unsigned int */
    CTYPE_int,     /* signed int */
    CTYPE_ulong,   /* unsigned long int */
    CTYPE_long,    /* signed long int */
    CTYPE_float,   /* float */
    CTYPE_double,  /* double */
    CTYPE_ldouble, /* long double */
} c4snet_type_t;

void C4SNetInit(int id, snet_distrib_t distImpl);

c4snet_data_t *C4SNetAlloc(c4snet_type_t type, size_t size, void **data);
c4snet_data_t *C4SNetCreate(c4snet_type_t type, size_t size, const void *data);
void C4SNetFree(c4snet_data_t *ptr);

c4snet_data_t *C4SNetShallowCopy(c4snet_data_t *ptr);
c4snet_data_t *C4SNetDeepCopy(c4snet_data_t *ptr);

int C4SNetSizeof(c4snet_data_t *data);
size_t C4SNetArraySize(c4snet_data_t *data);
void *C4SNetGetData(c4snet_data_t *ptr);

void C4SNetOut(void *hnd, int variant, ...);
#endif /* _C4SNET_H_ */
