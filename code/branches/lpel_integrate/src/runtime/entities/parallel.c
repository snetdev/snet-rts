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

#include "parallel.h"
#include "handle.h"
#include "record.h"
#include "typeencode.h"
#include "collectors.h"
#include "memfun.h"
#include "debug.h"

#include "threading.h"

#include "task.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif /* DISTRIBUTED_SNET */

//#define PARALLEL_DEBUG

static bool ContainsName( int name, int *names, int num)
{
  int i;
  bool found = false;
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




static match_count_t *CheckMatch( snet_record_t *rec, 
    snet_typeencoding_t *tenc, match_count_t *mc)
{
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
  /* for all variants */
  for( j=0; j<SNetTencGetNumVariants( tenc); j++) {
    venc = SNetTencGetVariant( tenc, j+1);
    MC_COUNT( mc) = 0;
    MC_ISMATCH( mc) = true;

    if( ( SNetRecGetNumFields( rec) < SNetTencGetNumFields( venc)) ||
        ( SNetRecGetNumTags( rec) < SNetTencGetNumTags( venc)) ||
        ( SNetRecGetNumBTags( rec) != SNetTencGetNumBTags( venc))) {
      MC_ISMATCH( mc) = false;
    } else {
      /* is_match is set to value inside the macros */
      FIND_NAME_LOOP( SNetRecGetNumFields, SNetTencGetNumFields,
          SNetRecGetFieldNames, SNetTencGetFieldNames);
      FIND_NAME_LOOP( SNetRecGetNumTags, SNetTencGetNumTags, 
          SNetRecGetTagNames, SNetTencGetTagNames);

      for( i=0; i<SNetRecGetNumBTags( rec); i++) {
        if(!SNetTencContainsBTagName(venc, SNetRecGetBTagNames(rec)[i])) {
          MC_ISMATCH( mc) = false;
        } else {
          MC_COUNT( mc) += 1;
        }
      }
    }
    if( MC_ISMATCH( mc)) {
      max = MC_COUNT( mc) > max ? MC_COUNT( mc) : max;
    }
  } /* end for all variants */

  if( max >= 0) {
    MC_ISMATCH( mc) = true;
    MC_COUNT( mc) = max;
  }
  
  return( mc);
}

/**
 * Check for "best match" and decide which buffer to dispatch to
 * in case of a draw.
 */
static int BestMatch( match_count_t **counter, int num)
{
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

/**
 * Write a record to the buffers.
 * @param counter   pointer to a counter -
 *                  if NULL, no sort records are generated
 */
static void PutToBuffers( stream_mh_t **outstreams, int num,
    int idx, snet_record_t *rec, int *counter)
{

  /* write data record to target stream */
#ifdef PARALLEL_DEBUG
  SNetUtilDebugNotice(
      "PARALLEL %p: Writing record %p to stream %p",
      outstreams, rec, outstreams[idx]
      );
#endif
  StreamWrite( outstreams[idx], rec);

  /* for the deterministic variant, broadcast sort record afterwards */
  if( counter != NULL) {
    int i;
    for( i=0; i<num; i++) {
      StreamWrite( outstreams[i],
          SNetRecCreate( REC_sort_end, 0, *counter));
    }
    *counter += 1;
  }
}


/**
 * Main Parallel Box Task
 */
static void ParallelBoxTask( task_t *self, void *arg)
{
  snet_handle_t *hnd = (snet_handle_t*) arg;
  snet_typeencoding_list_t *types = SNetHndGetTypeList( hnd);
  /* the number of outputs */
  int num = SNetTencGetNumTypes( types);
  stream_mh_t *instream;
  stream_mh_t *outstreams[num];
  int i, stream_index;
  snet_record_t *rec;
  match_count_t **matchcounter;
  bool is_det;
  int num_init_branches = 0;
  bool terminate = false;
  int counter = 1;


#ifdef PARALLEL_DEBUG
  SNetUtilDebugNotice("(CREATION PARALLEL)");
#endif


  /* open instream for reading */
  instream = StreamOpen(self, SNetHndGetInput(hnd), 'r');

  /* open outstreams for writing */
  {
    stream_t **tmp = SNetHndGetOutputs( hnd);
    for (i=0; i<num; i++) {
      outstreams[i] = StreamOpen( self, tmp[i], 'w');
    }
    /* the mem region is not needed anymore */
    SNetMemFree( tmp);
  }

  is_det = SNetHndIsDet( hnd);

  /* Handle initialiser branches */
  for( i=0; i<num; i++) {
    if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( types, i)) == 0) {

      PutToBuffers( outstreams, num, i, 
          SNetRecCreate( REC_trigger_initialiser), 
          (is_det) ? &counter : NULL
          );
      /* after terminate, it is not necessary to send a sort record */
      PutToBuffers( outstreams, num, i, 
          SNetRecCreate( REC_terminate), 
          NULL
          );
      num_init_branches += 1; 
    }
  }

  switch( num - num_init_branches) {
    case 1: /* remove dispatcher from network ... */
      for( i=0; i<num; i++) { 
        if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( types, i)) > 0) {
          /* Put to buffers? Is it not clearer to send it to the remaining stream??? */
          PutToBuffers( outstreams, num, i, 
              SNetRecCreate( REC_sync, SNetHndGetInput(hnd)), 
              NULL
              );
        }
      }    
    case 0: /* and terminate */
      terminate = true;
      StreamClose( instream, false);
    break;
    default: ;/* or resume operation as normal */
  }

  matchcounter = SNetMemAlloc( num * sizeof( match_count_t*));
  for( i=0; i<num; i++) {
    matchcounter[i] = SNetMemAlloc( sizeof( match_count_t));
  }

  /* MAIN LOOP START */
  while( !terminate) {
#ifdef PARALLEL_DEBUG
    SNetUtilDebugNotice("PARALLEL %p: reading %p", outstreams, instream);
#endif
    /* read a record from the instream */
    rec = StreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        for( i=0; i<num; i++) {
          CheckMatch( rec, SNetTencGetTypeEncoding( types, i), matchcounter[i]);
        }

        stream_index = BestMatch( matchcounter, num);
        PutToBuffers( outstreams, num, stream_index, rec, (is_det)? &counter : NULL);
        break;

      case REC_sync:
        {
          stream_t *newstream = SNetRecGetStream( rec);
          StreamReplace( instream, newstream);
          SNetHndSetInput( hnd, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        SNetUtilDebugNotice("[PAR] Received REC_collect, destroying it\n");
        SNetRecDestroy( rec);
        break;

      case REC_sort_end:
        /* increase level */
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        for( i=0; i<num; i++) {
          /* send a copy to all but the last, the last gets the original */
          StreamWrite(
              outstreams[i],
              (i != (num-1)) ? SNetRecCopy( rec) : rec
              );
        }
        break;

      case REC_terminate:
        terminate = true;
        for( i=0; i<num; i++) {
          /* send a copy to all but the last, the last gets the original */
          StreamWrite(
              outstreams[i],
              (i != (num-1)) ? SNetRecCopy( rec) : rec
              );
          /* close instream: only destroy if not synch'ed before */
          StreamClose( instream, true);
        }
        /* note that no sort record needs to be appended */
        break;

      default:
        SNetUtilDebugNotice("[PAR] Unknown control rec destroyed (%d).\n",
            SNetRecGetDescriptor( rec));
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */


  /* close the outstreams */
  for( i=0; i<num; i++) {
    StreamClose( outstreams[i], false);
    SNetMemFree( matchcounter[i]);
  }
  SNetMemFree( matchcounter);

  //TODO ? SNetTencDestroyTypeEncodingList( types);
  SNetHndDestroy( hnd);
} /* END of PARALLEL BOX TASK */



static stream_t *SNetParallelStartup( stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_list_t *types,
    void **funs, bool is_det)
{

  int i;
  int num;
  snet_handle_t *hnd;
  stream_t *outstream;
  stream_t **transits;
  stream_t **collstreams;
  snet_startup_fun_t fun;

#ifdef DISTRIBUTED_SNET
  snet_routing_context_t *context;
  snet_routing_context_t *new_context;
  context = SNetInfoGetRoutingContext(info);
  instream = SNetRoutingContextUpdate(context, instream, location); 

  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Parallel created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */


    num = SNetTencGetNumTypes( types);
    collstreams = SNetMemAlloc( num * sizeof( stream_t*));
    transits = SNetMemAlloc( num * sizeof( stream_t*));


    /* create all branches */
    for( i=0; i<num; i++) {
      transits[i] = StreamCreate();
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
      collstreams[i] = (*fun)( transits[i]);
#endif /* DISTRIBUTED_SNET */
      
    }
    /* create collector with outstreams */
    outstream = CollectorCreate(num, collstreams);

    hnd = SNetHndCreate( HND_parallel, instream, transits, types, is_det);
    SNetEntitySpawn( ParallelBoxTask, (void*)hnd,
        (is_det)? ENTITY_parallel_det: ENTITY_parallel_nondet
        );
    
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

/**
 * Parallel creation function
 */
stream_t *SNetParallel( stream_t *instream,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
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
  return( SNetParallelStartup( instream, types, funs, false));
#endif /* DISTRIBUTED_SNET */

}




/**
 * Det Parallel creation function
 */
stream_t *SNetParallelDet( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
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
  return( SNetParallelStartup( inbuf, types, funs, true));
#endif /* DISTRIBUTED_SNET */
}


