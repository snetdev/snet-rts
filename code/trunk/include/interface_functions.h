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

#define SNET_INTERFACE_ERR     -1
#define SNET_INTERFACE_ERR_BUF -2

/* MPI_Pack data type information into buffer for sending.
 *
 * 1: IN:     comm
 * 2: IN:     data
 * 3: IN/OUT: buffer
 * 4: IN:     size of the buffer
 *
 * return: position in the buffer (Used as count for MPI_Send (as position from MPI_Pack))
 */

typedef int (*snet_mpi_serialize_type_fun_t)(MPI_Comm, void*, void*, int);

/* Prepare data for sending.
 *
 * 1: IN:  comm
 * 2: IN:  data
 * 3: OUT: MPI type (Used as type for MPI_Send)
 * 4: OUT: address of the buffer to send (Used as buffer for MPI_Send)
 * 5: OUT: optional return value that is passed to cleanup_fun
 *
 * return: count of elements of returned type (Used as count for MPI_Send)
 */ 

typedef int (*snet_mpi_pack_fun_t)(MPI_Comm, void*, MPI_Datatype*, void**, void**);

/* Free resources used in sending.
 *
 * 1: IN: type value returned by pack_fun.
 * 2: IN: optional return value of pack_fun.
 *
 * Note: Expected to free all resources (MPI type, possible buffer for packing,
 *       optional return value etc.) reserved by the pack_fun!
 */

typedef void (*snet_mpi_cleanup_fun_t)(MPI_Datatype, void*);

/* Deserialize data type from buffer.
 *
 * 1: IN:  comm
 * 2: IN:  buffer
 * 3: IN:  size of the buffer
 * 4: OUT: MPI type (Used as type for MPI_Recv)
 * 5: OUT: Optional return value that is passed to unpack_fun
 *
 * return: count of elements of returned type (Used as count for MPI_Recv)
 *
 */

typedef int (*snet_mpi_deserialize_type_fun_t)(MPI_Comm, void*, int, MPI_Datatype*, void**);

/* 1: IN: comm
 * 2: IN: buffer (buffer received by MPI_Recv, function must free this, or it can be used directly as the data)
 * 3: IN: MPI type returned by deserialize_type_fun (Function must free this)
 * 4: IN: count of elements of type
 * 5: IN: Optional return value returned by deserialize_type_fun (Function must free this)
 *
 * Return: data
 *
 * Note: Expected to free all resources (MPI type, possible buffer for packing if
 *       not used directly etc.) used in this and deserialize_type_fun.
 */

typedef void* (*snet_mpi_unpack_fun_t)(MPI_Comm, void*, MPI_Datatype, int, void*);

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
