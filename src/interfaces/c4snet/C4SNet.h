#ifndef _C4SNET_H_
#define _C4SNET_H_

#include <stddef.h>
#include "type.h"

/* The type of a field in a record. */
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
    CTYPE_pointer, /* pointer to void */
} c4snet_type_t;

/* Language interface initialization function. */
void C4SNetInit(int id, snet_distrib_t distImpl);

/* Allocate a new field with uninitialized data. */
c4snet_data_t *C4SNetAlloc(c4snet_type_t type, size_t size, void **data);

/* Create a new field and initialize the data member as a copy of 'data'. */
c4snet_data_t *C4SNetCreate(c4snet_type_t type, size_t size, const void *data);

/* Decrease the reference count to a field and deallocate when zero. */
void C4SNetFree(c4snet_data_t *ptr);

/* Increase the reference count to an existing field. */
c4snet_data_t *C4SNetShallowCopy(c4snet_data_t *ptr);

/* Create a new field as a copy of an existing field. */
c4snet_data_t *C4SNetDeepCopy(c4snet_data_t *ptr);

/* Get the size of the basic data type. */
int C4SNetSizeof(c4snet_data_t *data);

/* Get the number of array elements. */
size_t C4SNetArraySize(c4snet_data_t *data);

/* Get a pointer to the data. */
void *C4SNetGetData(c4snet_data_t *ptr);

/* Retrieve the type of the data. */
c4snet_type_t C4SNetGetType(c4snet_data_t *data);

/* Get a copy of an unterminated char array as a proper C string. */
char* C4SNetGetString(c4snet_data_t *data);

/* Create a new record according to box output specification number 'variant'. */
void C4SNetOut(void *hnd, int variant, ...);

#endif /* _C4SNET_H_ */

