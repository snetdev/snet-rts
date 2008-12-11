#include "interface_functions.h"
#include "debug.h"
#include "memfun.h"

/* hkr: TODO: consider if this can be removed *somehow*? */
snet_global_info_structure_t *snet_global = NULL;

void SNetGlobalSetFreeFun( snet_global_interface_functions_t *f,
			   snet_free_fun_t freefun)
{
  f->freefun = freefun;
}

void SNetGlobalSetCopyFun( snet_global_interface_functions_t *f,
			   snet_copy_fun_t copyfun)
{
  f->copyfun = copyfun;
}

snet_copy_fun_t SNetGlobalGetCopyFun( snet_global_interface_functions_t *f)
{
  return( f->copyfun);
}

snet_free_fun_t SNetGlobalGetFreeFun( snet_global_interface_functions_t *f)
{
  return( f->freefun);
}

void SNetGlobalSetSerializationFun( snet_global_interface_functions_t *f,
				    snet_serialise_fun_t serialisefun)
{
  f->serialisefun = serialisefun;
}

snet_serialise_fun_t SNetGlobalGetSerializationFun(snet_global_interface_functions_t *f)
{
  return( f->serialisefun);
}

void SNetGlobalSetDeserializationFun( snet_global_interface_functions_t *f,
				      snet_deserialise_fun_t deserialisefun)
{
  f->deserialisefun = deserialisefun;
}

snet_deserialise_fun_t SNetGlobalGetDeserializationFun( snet_global_interface_functions_t *f)
{
  return( f->deserialisefun);
}

void SNetGlobalSetEncodingFun( snet_global_interface_functions_t *f,
				 snet_encode_fun_t encodefun)
{
  f->encodefun = encodefun;
}

snet_encode_fun_t SNetGlobalGetEncodingFun(snet_global_interface_functions_t *f)
{
  return( f->encodefun);
}

void SNetGlobalSetDecodingFun( snet_global_interface_functions_t *f,
			       snet_decode_fun_t decodefun)
{
  f->decodefun = decodefun;;
}

snet_decode_fun_t SNetGlobalGetDecodingFun( snet_global_interface_functions_t *f)
{
  return( f->decodefun);
}

void SNetGlobalSetId( snet_global_interface_functions_t *f, int id)
{
  f->id = id;
}

int SNetGlobalGetId( snet_global_interface_functions_t *f)
{
  return( f->id);
}



snet_global_interface_functions_t *SNetGlobalGetInterface( int id) 
{
  int i;
  snet_global_interface_functions_t *result = NULL;

  for( i=0; i<snet_global->num; i++) {
    if( SNetGlobalGetId( snet_global->interface[i]) == id) {
      result = snet_global->interface[i];
    }
  }
  if( result == NULL) {
    SNetUtilDebugFatal("Interface ID (%d) not found!\n\n", id);
  }

  return( result);
}


bool SNetGlobalRuntimeInitialised()
{
  if( snet_global == NULL) {
    fprintf( stderr, 
             "\n\n ** Fatal Error ** : Runtime System not initialised!\n\n");
    exit( 1);
  }
  return( true);
}

bool SNetGlobalInitialise() 
{
  bool success = false;
#ifdef DBG_RT_TRACE_INIT
  struct timeval t;
#endif
#ifdef DBG_RT_TRACE_THREAD_CREATE
  t_count_mtx = SNetMemAlloc( sizeof( pthread_mutex_t));
  pthread_mutex_init( t_count_mtx, NULL);
#endif

  if( snet_global == NULL) {
    snet_global = SNetMemAlloc( sizeof( snet_global_info_structure_t));
    snet_global->interface =
      SNetMemAlloc( 
          INITIAL_INTERFACE_TABLE_SIZE * 
          sizeof( snet_global_interface_functions_t*));
    snet_global->num = 0;
    success = true;
  }
  else {
    SNetUtilDebugFatal("[Global] Runtime system already initialized");
  }
#ifdef DBG_RT_TRACE_INIT
  gettimeofday( &t, NULL);
  SNetUtilDebugNotice("[DBG::RT::Global] Runtime system initialisesd at %lf\n",
                        t.tv_sec + t.tv_usec / 1000000.0);
#endif
  return( success);
}

void SNetGlobalDestroy() 
{
  int i;
#ifdef DBG_RT_TRACE_THREAD_CREATE
  pthread_mutex_destroy( t_count_mtx);
  SNetMemFree(t_count_mtx);
#endif

  if( snet_global != NULL) {
    for(i = 0; i < snet_global->num; i++) {
      SNetMemFree(snet_global->interface[i]);
    }

    SNetMemFree(snet_global->interface);
    SNetMemFree(snet_global); 
  }
  else {
    SNetUtilDebugFatal("[Global] Runtime system not initialized");
  }
#ifdef DBG_RT_TRACE_INIT
  gettimeofday( &t, NULL);
  SNetUtilDebugNotice("[DBG::RT::Global] Runtime system destroyed at %lf\n",
                        t.tv_sec + t.tv_usec / 1000000.0);
#endif
  return;
}

bool 
SNetGlobalRegisterInterface( int id, 
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
			     snet_mpi_cleanup_fun_t cleanupfun)
#else
                             snet_decode_fun_t decodefun)
#endif /* DISTRIBUTED_SNET */    
{
  int num; 
  snet_global_interface_functions_t *new_if;
  
  SNetGlobalRuntimeInitialised();
  num = snet_global->num;
  if( num == ( INITIAL_INTERFACE_TABLE_SIZE -1)) {
    /* TODO replace this with proper code!!!! */
    SNetUtilDebugFatal("[Global] Lookup table is full!\n\n");
  }
  else {
    new_if = SNetMemAlloc( sizeof( snet_global_interface_functions_t));  
    SNetGlobalSetId( new_if, id);
    SNetGlobalSetFreeFun( new_if, freefun);
    SNetGlobalSetCopyFun( new_if, copyfun);  
    SNetGlobalSetSerializationFun( new_if, serialisefun);
    SNetGlobalSetDeserializationFun( new_if, deserialisefun);  
    SNetGlobalSetEncodingFun( new_if, encodefun);
    SNetGlobalSetDecodingFun( new_if, decodefun);
#ifdef DISTRIBUTED_SNET
    SNetGlobalSetSerializeTypeFun( new_if, serialize_type_fun);
    SNetGlobalSetDeserializeTypeFun( new_if, desrialize_type_fun);
    SNetGlobalSetPackFun( new_if, packfun);
    SNetGlobalSetUnpackFun( new_if, unpackfun);
    SNetGlobalSetCleanupFun( new_if, cleanupfun);
#endif /* DISTRIBUTED_SNET */

    snet_global->interface[num] = new_if;
    snet_global->num += 1;
  }

  return( true);
}

snet_free_fun_t SNetGetFreeFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetFreeFun( SNetGlobalGetInterface( 
                    SNetRecGetInterfaceId( rec))));
}

snet_copy_fun_t SNetGetCopyFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetCopyFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

snet_copy_fun_t SNetGetCopyFun( int id) 
{
  return( SNetGlobalGetCopyFun( SNetGlobalGetInterface( id)));
}

snet_free_fun_t SNetGetFreeFun( int id) 
{
  return( SNetGlobalGetFreeFun( SNetGlobalGetInterface( id)));
}

snet_serialise_fun_t SNetGetSerializationFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetSerializationFun( SNetGlobalGetInterface( 
                    SNetRecGetInterfaceId( rec))));
}

snet_serialise_fun_t SNetGetSerializationFun( int id) 
{
  return( SNetGlobalGetSerializationFun( SNetGlobalGetInterface( id)));
}


snet_deserialise_fun_t SNetGetDeserializationFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetDeserializationFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

snet_deserialise_fun_t SNetGetDeserializationFun( int id) 
{
  return( SNetGlobalGetDeserializationFun( SNetGlobalGetInterface( id)));
}

snet_encode_fun_t SNetGetEncodingFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetEncodingFun( SNetGlobalGetInterface( 
		SNetRecGetInterfaceId( rec))));
}

snet_encode_fun_t SNetGetEncodingFun( int id) 
{
  return( SNetGlobalGetEncodingFun( SNetGlobalGetInterface( id)));
}


snet_decode_fun_t SNetGetDecodingFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetDecodingFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

snet_decode_fun_t SNetGetDecodingFun( int id) 
{
  return( SNetGlobalGetDecodingFun( SNetGlobalGetInterface( id)));
}

#ifdef DISTRIBUTED_SNET

void SNetGlobalSetSerializeTypeFun(snet_global_interface_functions_t *f, snet_mpi_serialize_type_fun_t serialize_type_fun)
{
  f->serialize_type_fun = serialize_type_fun;
}

snet_mpi_serialize_type_fun_t SNetGlobalGetSerializeTypeFun(snet_global_interface_functions_t *f)
{
  return( f->serialize_type_fun);
}

snet_mpi_serialize_type_fun_t SNetGetSerializeTypeFunFromRec(snet_record_t *rec)
{
  return( SNetGlobalGetSerializeTypeFun( SNetGlobalGetInterface( SNetRecGetInterfaceId( rec))));
}

snet_mpi_serialize_type_fun_t SNetGetSerializeTypeFun(int id)
{
  return( SNetGlobalGetSerializeTypeFun( SNetGlobalGetInterface( id)));
}


void SNetGlobalSetDeserializeTypeFun(snet_global_interface_functions_t *f, snet_mpi_deserialize_type_fun_t desrialize_type_fun)
{
  f->desrialize_type_fun = desrialize_type_fun;
}

snet_mpi_deserialize_type_fun_t SNetGlobalGetDeserializeTypeFun(snet_global_interface_functions_t *f)
{
  return( f->desrialize_type_fun);
}

snet_mpi_deserialize_type_fun_t SNetGetDeserializeTypeFunFromRec(snet_record_t *rec)
{
  return( SNetGlobalGetDeserializeTypeFun( SNetGlobalGetInterface( SNetRecGetInterfaceId( rec))));
}

snet_mpi_deserialize_type_fun_t SNetGetDeserializeTypeFun(int id)
{
  return( SNetGlobalGetDeserializeTypeFun( SNetGlobalGetInterface( id)));
}


void SNetGlobalSetPackFun(snet_global_interface_functions_t *f, snet_mpi_pack_fun_t packfun)
{
  f->packfun = packfun;
}

snet_mpi_pack_fun_t SNetGlobalGetPackFun(snet_global_interface_functions_t *f)
{
  return( f->packfun);
}

snet_mpi_pack_fun_t SNetGetPackFunFromRec(snet_record_t *rec)
{
  return( SNetGlobalGetPackFun( SNetGlobalGetInterface( SNetRecGetInterfaceId( rec))));
}

snet_mpi_pack_fun_t SNetGetPackFun(int id)
{
  return( SNetGlobalGetPackFun( SNetGlobalGetInterface( id)));
}


void SNetGlobalSetUnpackFun(snet_global_interface_functions_t *f, snet_mpi_unpack_fun_t unpackfun)
{
  f->unpackfun = unpackfun;
}

snet_mpi_unpack_fun_t SNetGlobalGetUnpackFun(snet_global_interface_functions_t *f)
{
  return( f->unpackfun);
}

snet_mpi_unpack_fun_t SNetGetUnpackFunFromRec(snet_record_t *rec)
{
  return( SNetGlobalGetUnpackFun( SNetGlobalGetInterface( SNetRecGetInterfaceId( rec))));
}

snet_mpi_unpack_fun_t SNetGetUnpackFun(int id)
{
  return( SNetGlobalGetUnpackFun( SNetGlobalGetInterface( id)));
}

void SNetGlobalSetCleanupFun(snet_global_interface_functions_t *f, snet_mpi_cleanup_fun_t cleanupfun)
{
  f->cleanupfun = cleanupfun;
}

snet_mpi_cleanup_fun_t SNetGlobalGetCleanupFun(snet_global_interface_functions_t *f)
{
  return( f->cleanupfun);
}

snet_mpi_cleanup_fun_t SNetGetCleanupFunFromRec(snet_record_t *rec)
{
  return( SNetGlobalGetCleanupFun( SNetGlobalGetInterface( SNetRecGetInterfaceId( rec))));
}

snet_mpi_cleanup_fun_t SNetGetCleanupFun(int id)
{
  return( SNetGlobalGetCleanupFun( SNetGlobalGetInterface( id)));
}

#endif /* DISTRIBUTED_SNET */

/* END -- GLOBALS                                                            */
/* ************************************************************************* */
