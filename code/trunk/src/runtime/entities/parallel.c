/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file parallel.c
 *
 * This implements the choice dispatcher. [...]
 *
 * Special handling for initialiser boxes is also implemented here.
 * Initialiser boxes are instantiated before the dispatcher enters its main 
 * event loop. With respect to outbound streams, initialiser boxes are handled
 * in the same way as  ordinary boxes. Inbound stream handling is different 
 * though: A trigger record (REC_trigger_initialiser) followed by a termination
 * record is sent to each initialiser box. The trigger record activates the 
 * initialiser box once, the termination record removes the initialiser from 
 * the network. The implementation ensures that a dispatcher removes itself
 * from the network if none or only one branch remain  after serving 
 * initialisation purposes:
 * If all branches of the dispatcher are initialiser boxes, the dispatcher 
 * exits after sending the trigger  and termination records. If there is one
 * ordinary branch  left, the dispatcher sends on its own inbound stream to
 * this branch  (REC_sync)  and exits afterwards. If more than one ordinary
 * boxes are left,  the dispatcher starts its main event loop as usual.
 * 
 *****************************************************************************/

#include "snetentities.h"
#include "handle_p.h"
#include "record_p.h"
#include "typeencode.h"
#include "collectors.h"
#include "memfun.h"
#include "debug.h"
#include "threading.h"
#include "stream_layer.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif /* DISTRIBUTED_SNET */

//#define PARALLEL_DEBUG

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct { 
  bool is_match;
  int match_count;
} match_count_t;

static bool ContainsName( int name, int *names, int num) {  
  int i;
  bool found;

  found = false;

  for( i=0; i<num; i++) {
    if( names[i] == name) {
      found = true;
      break;
    }
  }

  return( found);
}

/* ------------------------------------------------------------------------- */
/*  SNetParallel                                                             */
/* ------------------------------------------------------------------------- */

#define FIND_NAME_LOOP( RECNUM, TENCNUM, RECNAMES, TENCNAMES)\
for( i=0; i<TENCNUM( venc); i++) {\
       if( !( ContainsName( TENCNAMES( venc)[i], RECNAMES( rec), RECNUM( rec)))) {\
        MC_ISMATCH( mc) = false;\
      }\
      else {\
        MC_COUNT( mc) += 1;\
      }\
    }




static match_count_t 
*CheckMatch( snet_record_t *rec, 
             snet_typeencoding_t *tenc, 
             match_count_t *mc) {

  snet_variantencoding_t *venc;
  int i,j,max=-1;

  if(rec == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: rec == NULL");
  }
  if(tenc == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: tenc == NULL");
  }
  if(mc == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: mc == NULL");
  }

  for( j=0; j<SNetTencGetNumVariants( tenc); j++) {
    venc = SNetTencGetVariant( tenc, j+1);
    MC_COUNT( mc) = 0;
    MC_ISMATCH( mc) = true;

    if( ( SNetRecGetNumFields( rec) < SNetTencGetNumFields( venc)) ||
        ( SNetRecGetNumTags( rec) < SNetTencGetNumTags( venc)) ||
        ( SNetRecGetNumBTags( rec) != SNetTencGetNumBTags( venc))) {
      MC_ISMATCH( mc) = false;
    }
    else { // is_match is set to value inside the macros
      FIND_NAME_LOOP( SNetRecGetNumFields, SNetTencGetNumFields,
          SNetRecGetFieldNames, SNetTencGetFieldNames);
      FIND_NAME_LOOP( SNetRecGetNumTags, SNetTencGetNumTags, 
          SNetRecGetTagNames, SNetTencGetTagNames);
  
      for( i=0; i<SNetRecGetNumBTags( rec); i++) {
         if(!SNetTencContainsBTagName(venc, SNetRecGetBTagNames(rec)[i])) {
          MC_ISMATCH( mc) = false;
        }
        else {
         MC_COUNT( mc) += 1;
        }
      }
    }
    if( MC_ISMATCH( mc)) {
      max = MC_COUNT( mc) > max ? MC_COUNT( mc) : max;
    }
  } // for all variants

  if( max >= 0) {
    MC_ISMATCH( mc) = true;
    MC_COUNT( mc) = max;
  }
  
  return( mc);
}

// Checks for "best match" and decides which buffer to dispatch to
// in case of a draw.
static int BestMatch( match_count_t **counter, int num) {

  int i;
  int res, max;
  
  res = -1;
  max = -1;
  for( i=0; i<num; i++) {
    if( MC_ISMATCH( counter[i])) {
      if( MC_COUNT( counter[i]) > max) {
        res = i;
        max = MC_COUNT( counter[i]);
      }
    }
  }

  return( res);
}


static void
PutToBuffers(snet_tl_stream_t **streams,
             int num,
             int idx,
             snet_record_t *rec,
             int counter,
             bool det)
{

  int i;

  for( i=0; i<num; i++) {
    if( det) {
      SNetTlWrite( streams[i], SNetRecCreate( REC_sort_begin, 0, counter));
    }

    if( i == idx) {
#ifdef PARALLEL_DEBUG
      SNetUtilDebugNotice("PARALLEL %p: Writing record %p to stream %p",
                          streams, 
			  rec,
                          streams[i]);
#endif
      SNetTlWrite( streams[i], rec);
    }

    if( det) {
      SNetTlWrite( streams[i], SNetRecCreate( REC_sort_end, 0, counter));
    }
  }
}

static void *ParallelBoxThread( void *hndl) {

  int i, num, stream_index;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_record_t *rec;
  snet_tl_stream_t *go_stream = NULL;
  match_count_t **matchcounter;
  snet_tl_stream_t **streams;
  snet_typeencoding_list_t *types;

  bool is_det = SNetHndIsDet( hnd);
  
  int num_init_branches = 0;
  bool terminate = false;
  int counter = 1;


#ifdef PARALLEL_DEBUG
  SNetUtilDebugNotice("(CREATION PARALLEL)");
#endif

  streams = SNetHndGetOutputs( hnd);

  types = SNetHndGetTypeList( hnd);
  num = SNetTencGetNumTypes( types);

  /* Handle initialiser branches */
  for( i=0; i<num; i++) {
    if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( types, i)) == 0) {

      PutToBuffers( streams, 
                    num, 
                    i, 
                    SNetRecCreate( REC_trigger_initialiser), 
                    counter, 
                    is_det);
      counter += 1;
      PutToBuffers( streams, 
                    num, 
                    i, 
                    SNetRecCreate( REC_terminate), 
                    counter, 
                    is_det);
      counter += 1;
      num_init_branches += 1; 
    }
  }

  switch( num - num_init_branches) {
    case 1: /* remove dispatcher from network ... */
      for( i=0; i<num; i++) { 
        if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( types, i)) > 0) {
          PutToBuffers( streams, 
                        num, 
                        i, 
                        SNetRecCreate( REC_sync, SNetHndGetInput( hnd)), 
                        counter, 
                        is_det);
        }
      }    
    case 0: /* and terminate */
      terminate = true;
    break;

    default: /* or resume operation as normal */
    break;
  }



  matchcounter = SNetMemAlloc( num * sizeof( match_count_t*));

  for( i=0; i<num; i++) {
    matchcounter[i] = SNetMemAlloc( sizeof( match_count_t));
  }

  while( !( terminate)) {
#ifdef PARALLEL_DEBUG
    SNetUtilDebugNotice("PARALLEL %p: reading %p",
                        streams,
                        SNetHndGetInput(hnd));
#endif
    rec = SNetTlRead( SNetHndGetInput( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        for( i=0; i<num; i++) {
          CheckMatch( rec, SNetTencGetTypeEncoding( types, i), matchcounter[i]);
        }
 
        stream_index = BestMatch( matchcounter, num);
        go_stream = streams[stream_index];
        PutToBuffers( streams, num, stream_index, rec, counter, is_det);
        counter += 1;
      break;
      case REC_sync:
        SNetHndSetInput( hnd, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        SNetUtilDebugNotice("[Parallel] Unhandled control record, destroying" 
                            " it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
      case REC_sort_end:
        if( is_det) {
          SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        }
        for( i=0; i<(num-1); i++) {

          SNetTlWrite( streams[i], SNetRecCopy( rec));
        }
        SNetTlWrite( streams[num-1], rec);
        break;
      case REC_terminate:
        terminate = true;
        if( is_det) {
          for( i=0; i<num; i++) {
            SNetTlWrite( streams[i], 
                SNetRecCreate( REC_sort_begin, 0, counter));
          }
        }
        for( i=0; i<num; i++) {
          if( i == (num-1)) {
            SNetTlWrite( streams[i], rec);
          }
          else {
            SNetTlWrite( streams[i], SNetRecCopy( rec));
          }
        }
        for( i=0; i<num; i++) {
          SNetTlMarkObsolete(streams[i]);
          SNetMemFree( matchcounter[i]);
        }
        SNetMemFree( streams);
        SNetMemFree( matchcounter);
        SNetHndDestroy( hnd);
        break;
      case REC_probe:
        if(is_det) {
          for(i = 0; i<num; i++) {
            SNetTlWrite(streams[i],
                      SNetRecCreate(REC_sort_begin, 0, counter));
          }
        }
        for(i = 0; i<num;i++) {
          if(i==(num-1)) {
            SNetTlWrite(streams[i], rec);
          } else {
            SNetTlWrite(streams[i], SNetRecCopy(rec));
          }
        }
        break;
    default:
      SNetUtilDebugNotice("[Parallel] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
      SNetRecDestroy( rec);
      break;
    }
  }

  return( NULL);
}



static snet_tl_stream_t *SNetParallelStartup( snet_tl_stream_t *instream,
					      snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
					      int location,
#endif /* DISTRIBUTED_SNET */
					      snet_typeencoding_list_t *types,
					      void **funs, bool is_det)
{

  int i;
  int num;
  snet_handle_t *hnd;
  snet_tl_stream_t *outstream;
  snet_tl_stream_t **transits;
  snet_tl_stream_t **outstreams;
  snet_startup_fun_t fun;
  snet_entity_id_t my_id;

#ifdef DISTRIBUTED_SNET
  snet_routing_context_t *context;
  snet_routing_context_t *new_context;

  context = SNetInfoGetRoutingContext(info);

  instream = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), instream, location); 

  if(location == SNetIDServiceGetNodeID()) {

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Parallel created");
#endif /* DISTRIBUTED_DEBUG */

#endif /* DISTRIBUTED_SNET */

    num = SNetTencGetNumTypes( types);
    outstreams = SNetMemAlloc( num * sizeof( snet_tl_stream_t*));
    transits = SNetMemAlloc( num * sizeof( snet_tl_stream_t*));
    
    transits[0] = SNetTlCreateStream( BUFFER_SIZE);
    
    fun = funs[0];
    
#ifdef DISTRIBUTED_SNET
    new_context =  SNetRoutingContextCopy(context);
    SNetRoutingContextSetLocation(new_context, location);
    SNetRoutingContextSetParent(new_context, location);

    SNetInfoSetRoutingContext(info, new_context);

    outstreams[0] = (*fun)(transits[0], info, location);

    outstreams[0] = SNetRoutingContextEnd(new_context, outstreams[0]);

    SNetRoutingContextDestroy(new_context);
#else
    outstreams[0] = (*fun)( transits[0], info);
#endif /* DISTRIBUTED_SNET */
    
    outstream = CreateDetCollector( outstreams[0]);
    my_id = is_det ? ENTITY_parallel_det : ENTITY_parallel_nondet;
    if( is_det) {
      SNetTlWrite(outstreams[0], SNetRecCreate( REC_sort_begin, 0, 0));
    }
    
    for( i=1; i<num; i++) {
      transits[i] = SNetTlCreateStream( BUFFER_SIZE);
      
      fun = funs[i];
      
#ifdef DISTRIBUTED_SNET
      new_context = SNetRoutingContextCopy(context);
      SNetRoutingContextSetLocation(new_context, location);
      SNetRoutingContextSetParent(new_context, location);

      SNetInfoSetRoutingContext(info, new_context);
    
      outstreams[i] = (*fun)(transits[i], info, location);

      outstreams[i] = SNetRoutingContextEnd(new_context, outstreams[i]);
   
      SNetRoutingContextDestroy(new_context);

#else
      outstreams[i] = (*fun)( transits[i], info);
#endif /* DISTRIBUTED_SNET */
      
      SNetTlWrite( outstreams[0], SNetRecCreate( REC_collect, outstreams[i]));
      if( is_det) {
	SNetTlWrite( outstreams[i], SNetRecCreate( REC_sort_end, 0, 0));
      }
    }
    
    if( is_det) {
      SNetTlWrite( outstreams[0], SNetRecCreate( REC_sort_end, 0, 0));
    }
    hnd = SNetHndCreate( HND_parallel, instream, transits, types, is_det);
    
#ifdef PARALLEL_DEBUG
    SNetUtilDebugNotice("-");
    SNetUtilDebugNotice("| PARALLEL CREATED");
    SNetUtilDebugNotice("| input: %p", SNetHndGetInput(hnd));
    SNetUtilDebugNotice("| internal network inputs::");
    for(i = 0; i < num; i++) {
    SNetUtilDebugNotice("| - %p", transits[i]);
    }
    SNetUtilDebugNotice("-");
#endif
    SNetTlCreateComponent(ParallelBoxThread, (void*)hnd, my_id);
    
    SNetMemFree(outstreams);
    
#ifdef DISTRIBUTED_SNET
  } else { 

    num = SNetTencGetNumTypes( types); 

    for(i = 0; i < num; i++) { 
      fun = funs[i];

      new_context =  SNetRoutingContextCopy(context);
      SNetRoutingContextSetLocation(new_context, location);
      SNetRoutingContextSetParent(new_context, location);

      SNetInfoSetRoutingContext(info, new_context);
    
      instream = (*fun)( instream, info, location);

      instream  = SNetRoutingContextEnd(new_context, instream);

      SNetRoutingContextDestroy(new_context);
    } 

    SNetTencDestroyTypeEncodingList( types);

    outstream  = instream; 
  }

  SNetInfoSetRoutingContext(info, context);
#endif /* DISTRIBUTED_SNET */
  
  SNetMemFree( funs);
  
  return( outstream);
}


extern snet_tl_stream_t *SNetParallel( snet_tl_stream_t *instream,
				       snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
				       int location,
#endif /* DISTRIBUTED_SNET */
				       snet_typeencoding_list_t *types,
				       ...)
{

  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumTypes( types);

  funs = SNetMemAlloc( num * sizeof( void*));

  va_start( args, types);

  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

#ifdef DISTRIBUTED_SNET
  return( SNetParallelStartup( instream, info, location, types, funs, false));
#else
  return( SNetParallelStartup( instream, info, types, funs, false));
#endif /* DISTRIBUTED_SNET */

}

extern snet_tl_stream_t *SNetParallelDet( snet_tl_stream_t *inbuf,
					  snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
					  int location,
#endif /* DISTRIBUTED_SNET */
					  snet_typeencoding_list_t *types,
					  ...)
{


  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumTypes( types);

  funs = SNetMemAlloc( num * sizeof( void*));

  va_start( args, types);

  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

#ifdef DISTRIBUTED_SNET
  return( SNetParallelStartup( inbuf, info, location, types, funs, true));
#else
  return( SNetParallelStartup( inbuf, info, types, funs, true));
#endif /* DISTRIBUTED_SNET */
}


