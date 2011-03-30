/** <!--********************************************************************-->
 *
 * $Id: parallel.c 2884 2010-10-06 20:09:11Z dlp $
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
#include "record_p.h"
#include "typeencode.h"
#include "collector.h"
#include "memfun.h"
#include "debug.h"
#include "distribution.h"

#include "threading.h"

typedef struct {
  snet_stream_t *input;
  snet_stream_t **outputs;
  snet_typeencoding_list_t *types;
  bool is_det;
} parallel_arg_t;

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct {
  bool is_match;
  int match_count;
} match_count_t;

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
static void PutToBuffers( snet_stream_desc_t **outstreams, int num,
    int idx, snet_record_t *rec, int *counter)
{

  /* write data record to target stream */
  if (outstreams[idx] != NULL) {
    SNetStreamWrite( outstreams[idx], rec);
  }

  /* for the deterministic variant, broadcast sort record afterwards */
  if( counter != NULL) {
    int i;
    for( i=0; i<num; i++) {
      if (outstreams[i] != NULL) {
        SNetStreamWrite( outstreams[i],
            SNetRecCreate( REC_sort_end, 0, *counter));
      }
    }
    *counter += 1;
  }
}




/**
 * Main Parallel Box Task
 */
static void ParallelBoxTask( snet_entity_t *self, void *arg)
{
  parallel_arg_t *parg = (parallel_arg_t *) arg;
  /* the number of outputs */
  int num = SNetTencGetNumTypes( parg->types);
  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstreams[num];
  int i, stream_index;
  snet_record_t *rec;
  match_count_t **matchcounter;
  int num_init_branches = 0;
  bool terminate = false;
  int counter = 1;


  /* open instream for reading */
  instream = SNetStreamOpen(self, parg->input, 'r');

  /* open outstreams for writing */
  {
    for (i=0; i<num; i++) {
      outstreams[i] = SNetStreamOpen( self, parg->outputs[i], 'w');
    }
    /* the mem region is not needed anymore */
    SNetMemFree( parg->outputs);
  }

  /* Handle initialiser branches */
  for( i=0; i<num; i++) {
    if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( parg->types, i)) == 0) {

      PutToBuffers( outstreams, num, i, 
          SNetRecCreate( REC_trigger_initialiser), 
          (parg->is_det) ? &counter : NULL
          );
      /* after terminate, it is not necessary to send a sort record */
      PutToBuffers( outstreams, num, i, 
          SNetRecCreate( REC_terminate), 
          NULL
          );
      SNetStreamClose( outstreams[i], false);
      outstreams[i] = NULL;
      num_init_branches += 1; 
    }
  }

  switch( num - num_init_branches) {
    case 1: /* remove dispatcher from network ... */
      for( i=0; i<num; i++) { 
        if (SNetTencGetNumVariants( SNetTencGetTypeEncoding( parg->types, i)) > 0) {
          /* send a sync record to the remaining branch */
          SNetStreamWrite( outstreams[i],
              SNetRecCreate( REC_sync, parg->input) 
              );
          SNetStreamClose( instream, false);
        }
      }    
    case 0: /* and terminate */
      terminate = true;
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
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        for( i=0; i<num; i++) {
          CheckMatch( rec, SNetTencGetTypeEncoding( parg->types, i), matchcounter[i]);
        }
        stream_index = BestMatch( matchcounter, num);
        PutToBuffers( outstreams, num, stream_index, rec, (parg->is_det)? &counter : NULL);
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream( rec);
          SNetStreamReplace( instream, newstream);
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
        /* send a copy to all */
        for( i=0; i<num; i++) {
          if (outstreams[i] != NULL) {
            SNetStreamWrite( outstreams[i], SNetRecCopy( rec));
          }
        }
        /* destroy the original */
        SNetRecDestroy( rec);
        break;

      case REC_terminate:
        terminate = true;
        /* send a copy to all */
        for( i=0; i<num; i++) {
          if (outstreams[i] != NULL) {
            SNetStreamWrite( outstreams[i], SNetRecCopy( rec));
          }
        }
        /* destroy the original */
        SNetRecDestroy( rec);
        /* close instream: only destroy if not synch'ed before */
        SNetStreamClose( instream, true);
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
    if (outstreams[i] != NULL) {
      SNetStreamClose( outstreams[i], false);
    }
    SNetMemFree( matchcounter[i]);
  }
  SNetMemFree( matchcounter);

  SNetTencDestroyTypeEncodingList( parg->types);
  SNetMemFree( parg);
} /* END of PARALLEL BOX TASK */




/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/


/**
 * Convenience function for creating parallel entities
 */
static snet_stream_t *CreateParallel( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_typeencoding_list_t *types,
    void **funs, bool is_det)
{
  int i;
  int num;
  parallel_arg_t *parg;
  snet_stream_t *outstream;
  snet_stream_t **transits;
  snet_stream_t **collstreams;
  snet_startup_fun_t fun;
  snet_info_t *new_info;

  num = SNetTencGetNumTypes( types);

  instream = SNetRouteUpdate(info, instream, location);
  if(location == SNetNodeLocation) {
    collstreams = SNetMemAlloc( num * sizeof( snet_stream_t*));
    transits = SNetMemAlloc( num * sizeof( snet_stream_t*));

    /* create all branches */
    for(i = 0; i < num; i++) {
      transits[i] = SNetStreamCreate(0);
      fun = funs[i];
      new_info = SNetInfoCopy(info);
      collstreams[i] = (*fun)(transits[i], new_info, location);
      SNetInfoDestroy(new_info);
    }
    /* create collector with outstreams */
    outstream = CollectorCreate(num, collstreams, info);

    parg = SNetMemAlloc( sizeof(parallel_arg_t));
    parg->input   = instream;
    parg->outputs = transits;
    parg->types = types;
    parg->is_det = is_det;

    SNetEntitySpawn( ENTITY_PARALLEL, ParallelBoxTask, (void*)parg );

  } else {
    for(i = 0; i < num; i++) {
      fun = funs[i];
      new_info = SNetInfoCopy(info);
      instream = (*fun)( instream, info, location);
      SNetInfoDestroy(new_info);
    }
    SNetTencDestroyTypeEncodingList( types);
    outstream = instream;
  }

  SNetMemFree( funs);
  return( outstream);
}




/**
 * Parallel creation function
 */
snet_stream_t *SNetParallel( snet_stream_t *instream,
    snet_info_t *info,
    int location,
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

  return CreateParallel( instream, info, location, types, funs, false);
}


/**
 * Det Parallel creation function
 */
snet_stream_t *SNetParallelDet( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
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

  return CreateParallel( inbuf, info, location, types, funs, true);
}
