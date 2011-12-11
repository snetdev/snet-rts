#ifndef INTERFACE_FUNCTIONS_HEADER
#define INTERFACE_FUNCTIONS_HEADER

#include <stdio.h>
#include "bool.h"

/* forward declaration */
struct record;

typedef void (*snet_free_fun_t)( void*);
typedef void* (*snet_copy_fun_t)( void*);
typedef size_t (*snet_alloc_size_fun_t)( void*);
typedef int (*snet_serialise_fun_t)( FILE *, void*);
typedef void* (*snet_deserialise_fun_t)( FILE *);
typedef int (*snet_encode_fun_t)( FILE *, void*);
typedef void* (*snet_decode_fun_t)( FILE *);
typedef void (*snet_pack_fun_t)(void *data, void *arg);
typedef void *(*snet_unpack_fun_t)(void *arg);

#define SNET_INTERFACE_ERR     -1
#define SNET_INTERFACE_ERR_BUF -2
typedef struct SNET_INTERFACE {
  int id;
  struct SNET_INTERFACE *next;

  snet_free_fun_t freefun;
  snet_copy_fun_t copyfun;
  snet_alloc_size_fun_t allocsizefun;
  snet_serialise_fun_t serialisefun;
  snet_deserialise_fun_t deserialisefun;
  snet_encode_fun_t encodefun;
  snet_decode_fun_t decodefun;
  snet_pack_fun_t packfun;
  snet_unpack_fun_t unpackfun;
} snet_interface_functions_t;

void SNetInterfaceRegister( int id,
                            snet_free_fun_t freefun,
                            snet_copy_fun_t copyfun,
                            snet_alloc_size_fun_t allocsizefun,
                            snet_serialise_fun_t serialisefun,
                            snet_deserialise_fun_t deserialisefun,
                            snet_encode_fun_t encodefun,
                            snet_decode_fun_t decodefun,
                            snet_pack_fun_t distPackFun,
                            snet_unpack_fun_t distUnpackFun);

snet_interface_functions_t *SNetInterfaceGet(int id);
void SNetInterfacesDestroy();
#endif
