#include "interface_functions.h"
#include "debug.h"
#include "memfun.h"

/* hkr: TODO: consider if this can be removed *somehow*? */
snet_global_info_structure_t *snet_global = NULL;

void SNetGlobalSetFreeFun( snet_global_interface_functions_t *f,
                        void (*freefun)( void*))
{
  f->freefun = freefun;
}

void SNetGlobalSetCopyFun( snet_global_interface_functions_t *f,
                        void* (*copyfun)( void*))
{
  f->copyfun = copyfun;
}

void *SNetGlobalGetCopyFun( snet_global_interface_functions_t *f)
{
  return( f->copyfun);
}

void *SNetGlobalGetFreeFun( snet_global_interface_functions_t *f)
{
  return( f->freefun);
}

void SNetGlobalSetSerializationFun( snet_global_interface_functions_t *f,
				 int (*serialisefun)( FILE *, void *))
{
  f->serialisefun = serialisefun;
}

void *SNetGlobalGetSerializationFun(snet_global_interface_functions_t *f)
{
  return( f->serialisefun);
}

void SNetGlobalSetDeserializationFun( snet_global_interface_functions_t *f,
				   void* (*deserialisefun)(FILE *))
{
  f->deserialisefun = deserialisefun;
}

void *SNetGlobalGetDeserializationFun( snet_global_interface_functions_t *f)
{
  return( f->deserialisefun);
}

void SNetGlobalSetEncodingFun( snet_global_interface_functions_t *f,
				 int (*encodefun)( FILE *, void *))
{
  f->encodefun = encodefun;
}

void *SNetGlobalGetEncodingFun(snet_global_interface_functions_t *f)
{
  return( f->encodefun);
}

void SNetGlobalSetDecodingFun( snet_global_interface_functions_t *f,
			       void* (*decodefun)(FILE *))
{
  f->decodefun = decodefun;;
}

void *SNetGlobalGetDecodingFun( snet_global_interface_functions_t *f)
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


bool 
SNetGlobalRegisterInterface( int id, 
                             void (*freefun)( void*),
                             void* (*copyfun)( void*), 
			     int (*serialisefun)( FILE *, void*),
			     void* (*deserialisefun)( FILE *),
			     int (*encodefun)( FILE *, void*),
			     void* (*decodefun)( FILE *))
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
    snet_global->interface[num] = new_if;
    snet_global->num += 1;
  }

  return( true);
}

void *SNetGetFreeFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetFreeFun( SNetGlobalGetInterface( 
                    SNetRecGetInterfaceId( rec))));
}

void *SNetGetCopyFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetCopyFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

void *SNetGetCopyFun( int id) 
{
  return( SNetGlobalGetCopyFun( SNetGlobalGetInterface( id)));
}

void *SNetGetFreeFun( int id) 
{
  return( SNetGlobalGetFreeFun( SNetGlobalGetInterface( id)));
}

void *SNetGetSerializationFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetSerializationFun( SNetGlobalGetInterface( 
                    SNetRecGetInterfaceId( rec))));
}

void *SNetGetSerializationFun( int id) 
{
  return( SNetGlobalGetSerializationFun( SNetGlobalGetInterface( id)));
}


void *SNetGetDeserializationFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetDeserializationFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

void *SNetGetDeserializationFun( int id) 
{
  return( SNetGlobalGetDeserializationFun( SNetGlobalGetInterface( id)));
}

void *SNetGetEncodingFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetEncodingFun( SNetGlobalGetInterface( 
		SNetRecGetInterfaceId( rec))));
}

void *SNetGetEncodingFun( int id) 
{
  return( SNetGlobalGetEncodingFun( SNetGlobalGetInterface( id)));
}


void *SNetGetDecodingFunFromRec( snet_record_t *rec) 
{
  return( SNetGlobalGetDecodingFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( rec))));
}

void *SNetGetDecodingFun( int id) 
{
  return( SNetGlobalGetDecodingFun( SNetGlobalGetInterface( id)));
}

/* END -- GLOBALS                                                            */
/* ************************************************************************* */
