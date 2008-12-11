#ifndef INTERFACE_FUNCTIONS_HEADER
#define INTERFACE_FUNCTIONS_HEADER

#include <stdio.h>
#include "bool.h"
#include "stdlib.h"
#include "pthread.h"
#include "record.h"

#ifdef DISTRIBUTED_SNET
#include <mpi.h>
#endif /* DISTRIBUTED_SNET */

typedef void (*snet_free_fun_t)( void*);
typedef void* (*snet_copy_fun_t)( void*);
typedef int (*snet_serialise_fun_t)( FILE *, void*);
typedef void* (*snet_deserialise_fun_t)( FILE *);
typedef int (*snet_encode_fun_t)( FILE *, void*);
typedef void* (*snet_decode_fun_t)( FILE *);

#ifdef DISTRIBUTED_SNET

typedef int (*snet_mpi_serialize_type_fun_t)(MPI_Comm, void *, void*, int);
typedef MPI_Datatype (*snet_mpi_deserialize_type_fun_t)(MPI_Comm, void *, int);

typedef void* (*snet_mpi_pack_fun_t)(MPI_Comm, void *, MPI_Datatype *, int *);
typedef void* (*snet_mpi_unpack_fun_t)(MPI_Comm, void *, MPI_Datatype, int);
typedef void (*snet_mpi_cleanup_fun_t)(MPI_Datatype, void*);
#endif /* DISTRIBUTED_SNET */

typedef struct {
  int id;
  snet_free_fun_t freefun;
  snet_copy_fun_t copyfun;
  snet_serialise_fun_t serialisefun;
  snet_deserialise_fun_t deserialisefun;
  snet_encode_fun_t encodefun;
  snet_decode_fun_t decodefun;
#ifdef DISTRIBUTED_SNET
  snet_mpi_serialize_type_fun_t serialize_type_fun;
  snet_mpi_deserialize_type_fun_t desrialize_type_fun;
  snet_mpi_pack_fun_t packfun;
  snet_mpi_unpack_fun_t unpackfun;
  snet_mpi_cleanup_fun_t cleanupfun;
#endif /* DISTRIBUTED_SNET */
} snet_global_interface_functions_t;

typedef struct {
  int num;
  snet_global_interface_functions_t **interface;
} snet_global_info_structure_t;

/* hkr: TODO: consider if this can be removed *somehow*? */
extern snet_global_info_structure_t *snet_global;

#ifdef DBG_RT_TRACE_THREAD_CREATE
  int tcount = 0;
  pthread_mutex_t *t_count_mtx;
  
  int SNetGlobalGetThreadCount();
  void SNetGlobalIncThreadCount();
  void SNetGlobalLockThreadMutex();
  void SNetGlobalUnlockThreadMutex();
#endif

bool SNetGlobalRegisterInterface( int id, 
				  snet_free_fun_t freefun,
				  snet_copy_fun_t copyfun,
				  snet_serialise_fun_t serialisefun,
				  snet_deserialise_fun_t deserialisefun,
				  snet_encode_fun_t encodefun,

#ifdef DISTRIBUTED_SNET
				  snet_decode_fun_t decodefun,
				  snet_mpi_serialize_type_fun_t serialize_type_fun,
				  snet_mpi_deserialize_type_fun_t desrialize_type_fun,
				  snet_mpi_pack_fun_t packfun,
				  snet_mpi_unpack_fun_t unpackfun,
				  snet_mpi_cleanup_fun_t cleanupfun);
#else
                                  snet_decode_fun_t decodefun);
#endif /* DISTRIBUTED_SNET */    



void SNetGlobalSetFreeFun(snet_global_interface_functions_t *f, 
			  snet_free_fun_t freefun);
snet_free_fun_t SnetGlobalGetFreeFun(snet_global_interface_functions_t *f);

void SNetGlobalSetCopyFun(snet_global_interface_functions_t *f,
			  snet_copy_fun_t copyfun);
snet_copy_fun_t SNetGlobalGetCopyFun(snet_global_interface_functions_t *f);

void SNetGlobalSetSerializationFun(snet_global_interface_functions_t *f,
				   snet_serialise_fun_t serialisefun);
snet_serialise_fun_t SNetGlobalGetSerializationFun(snet_global_interface_functions_t *f);

void SNetGlobalSetDeserializationFun(snet_global_interface_functions_t *f,
                        snet_deserialise_fun_t deserialisefun);

snet_deserialise_fun_t SNetGlobalGetDeserializationFun(snet_global_interface_functions_t *f);

void SNetGlobalSetEncodingFun(snet_global_interface_functions_t *f,
			      snet_encode_fun_t encodefun);
snet_encode_fun_t SNetGlobalGetEncodingFun(snet_global_interface_functions_t *f);

void SNetGlobalSetDecodingFun(snet_global_interface_functions_t *f,
			      snet_decode_fun_t decodefun);

snet_decode_fun_t SNetGlobalGetDecodingFun(snet_global_interface_functions_t *f);

snet_free_fun_t SNetGetFreeFunFromRec(snet_record_t *rec);
snet_copy_fun_t SNetGetCopyFunFromRec(snet_record_t *rec);
snet_serialise_fun_t SNetGetSerializationFunFromRec(snet_record_t *rec);
snet_deserialise_fun_t SNetGetDeserializationFunFromRec(snet_record_t *rec);
snet_encode_fun_t SNetGetEncodingFunFromRec(snet_record_t *rec);
snet_decode_fun_t SNetGetDecodingFunFromRec(snet_record_t *rec);

void SNetGlobalSetId(snet_global_interface_functions_t *f, int id);
int SNetGlobalGetId(snet_global_interface_functions_t *f);

snet_global_interface_functions_t *SNetGlobalGetInterface(int id);
bool SNetGlobalRuntimeInitialised();

bool SNetGlobalInitialise();
void SNetGlobalDestroy();

snet_copy_fun_t SNetGetCopyFun(int id);
snet_free_fun_t SNetGetFreeFun(int id);
snet_serialise_fun_t SNetGetSerializationFun(int id);
snet_deserialise_fun_t SNetGetDeserializationFun(int id);
snet_encode_fun_t SNetGetEncodingFun(int id);
snet_decode_fun_t SNetGetDecodingFun(int id);

#ifdef DISTRIBUTED_SNET

void SNetGlobalSetSerializeTypeFun(snet_global_interface_functions_t *f, snet_mpi_serialize_type_fun_t serialize_type_fun);
snet_mpi_serialize_type_fun_t SNetGlobalGetSerializeTypeFun(snet_global_interface_functions_t *f);
snet_mpi_serialize_type_fun_t SNetGetSerializeTypeFunFromRec(snet_record_t *rec);
snet_mpi_serialize_type_fun_t SNetGetSerializeTypeFun(int id);

void SNetGlobalSetDeserializeTypeFun(snet_global_interface_functions_t *f, snet_mpi_deserialize_type_fun_t desrialize_type_fun);
snet_mpi_deserialize_type_fun_t SNetGlobalGetDeserializeTypeFun(snet_global_interface_functions_t *f);
snet_mpi_deserialize_type_fun_t SNetGetDeserializeTypeFunFromRec(snet_record_t *rec);
snet_mpi_deserialize_type_fun_t SNetGetDeserializeTypeFun(int id);

void SNetGlobalSetPackFun(snet_global_interface_functions_t *f, snet_mpi_pack_fun_t packfun);
snet_mpi_pack_fun_t SNetGlobalGetPackFun(snet_global_interface_functions_t *f);
snet_mpi_pack_fun_t SNetGetPackFunFromRec(snet_record_t *rec);
snet_mpi_pack_fun_t SNetGetPackFun(int id);

void SNetGlobalSetUnpackFun(snet_global_interface_functions_t *f, snet_mpi_unpack_fun_t unpackfun);
snet_mpi_unpack_fun_t SNetGlobalGetUnpackFun(snet_global_interface_functions_t *f);
snet_mpi_unpack_fun_t SNetGetUnpackFunFromRec(snet_record_t *rec);
snet_mpi_unpack_fun_t SNetGetUnpackFun(int id);

void SNetGlobalSetCleanupFun(snet_global_interface_functions_t *f, snet_mpi_cleanup_fun_t cleanupfun);
snet_mpi_cleanup_fun_t SNetGlobalGetCleanupFun(snet_global_interface_functions_t *f);
snet_mpi_cleanup_fun_t SNetGetCleanupFunFromRec(snet_record_t *rec);
snet_mpi_cleanup_fun_t SNetGetCleanupFun(int id);

#endif /* DISTRIBUTED_SNET */
#endif
