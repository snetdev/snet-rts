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
#include "collector.h"
#include "memfun.h"
#include "debug.h"
#include "locvec.h"
#include "distribution.h"
#include "threading.h"

typedef struct {
  snet_stream_t *input;
  snet_stream_t **outputs;
  snet_variant_list_list_t *variant_lists;
  bool is_det;
} parallel_arg_t;

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct {
  bool is_match;
  int match_count;
} match_count_t;

/* ------------------------------------------------------------------------- */
/*  SNetParallel                                                             */
/* ------------------------------------------------------------------------- */

static match_count_t *CheckMatch( snet_record_t *rec,
    snet_variant_list_t *variant_list, match_count_t *mc)
{
  int name, val;
  snet_variant_t *variant;
  int max=-1;

  if(rec == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: rec == NULL");
  }
  if(variant_list == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: tenc == NULL");
  }
  if(mc == NULL) {
    SNetUtilDebugFatal("PARALLEL: CheckMatch: mc == NULL");
  }
  /* for all variants */
  LIST_FOR_EACH(variant_list, variant)
    MC_COUNT( mc) = 0;
    MC_ISMATCH( mc) = true;

    /* is_match is set to value inside the macros */
    VARIANT_FOR_EACH_FIELD(variant, name)
      if (!SNetRecHasField( rec, name)) {
        MC_ISMATCH( mc) = false;
      } else {
        MC_COUNT( mc) += 1;
      }
    END_FOR

    VARIANT_FOR_EACH_TAG(variant, name)
      if (!SNetRecHasTag( rec, name)) {
        MC_ISMATCH( mc) = false;
      } else {
        MC_COUNT( mc) += 1;
      }
    END_FOR

    FOR_EACH_BTAG(rec, name, val)
      if(!SNetVariantHasBTag(variant, name)) {
        MC_ISMATCH( mc) = false;
      } else {
        MC_COUNT( mc) += 1;
      }
    END_FOR

    if (MC_ISMATCH( mc)) {
      max = MC_COUNT( mc) > max ? MC_COUNT( mc) : max;
    }
  END_FOR

  if( max >= 0) {
    MC_ISMATCH( mc) = true;
    MC_COUNT( mc) = max;
  }

  return mc;
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
static void ParallelBoxTask(void *arg)
{
  parallel_arg_t *parg = (parallel_arg_t *) arg;
  /* the number of outputs */
  int num = SNetVariantListListLength(parg->variant_lists);
  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstreams[num];
  int i, stream_index;
  snet_record_t *rec;
  match_count_t **matchcounter;
  int num_init_branches = 0;
  bool terminate = false;
  int counter = 1;


  /* open instream for reading */
  instream = SNetStreamOpen(parg->input, 'r');

  /* open outstreams for writing */
  {
    for (i=0; i<num; i++) {
      outstreams[i] = SNetStreamOpen(parg->outputs[i], 'w');
    }
    /* the mem region is not needed anymore */
    SNetMemFree( parg->outputs);
  }

  /* Handle initialiser branches */
  for( i=0; i<num; i++) {
    if (SNetVariantListLength( SNetVariantListListGet( parg->variant_lists, i)) == 0) {

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
        if (SNetVariantListLength( SNetVariantListListGet( parg->variant_lists, i)) > 0) {
          /* send a sync record to the remaining branch */
          SNetStreamWrite( outstreams[i], SNetRecCreate( REC_sync, parg->input));
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
          CheckMatch( rec, SNetVariantListListGet( parg->variant_lists, i), matchcounter[i]);
        }
        stream_index = BestMatch( matchcounter, num);
        if (stream_index == -1) {
          SNetUtilDebugNotice("[PAR] Cannot route data record, no matching branch!\n");
        }
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

      case REC_source:
        /* ignore, destroy */
        SNetRecDestroy( rec);
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

  SNetVariantListListDestroy( parg->variant_lists);
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
    snet_variant_list_list_t *variant_lists,
    void **funs, bool is_det)
{
  int i;
  int num;
  parallel_arg_t *parg;
  snet_stream_t *outstream;
  snet_stream_t **transits;
  snet_stream_t **collstreams;
  snet_startup_fun_t fun;
  snet_variant_list_t *variants;
  snet_locvec_t *locvec;

  num = SNetVariantListListLength( variant_lists);

  locvec = SNetLocvecGet(info);
  SNetLocvecParallelEnter(locvec);

  instream = SNetRouteUpdate(info, instream, location);
  if(location == SNetNodeLocation) {
    transits    = SNetMemAlloc( num * sizeof( snet_stream_t*));
    collstreams = SNetMemAlloc( num * sizeof( snet_stream_t*));

    /* create branches */
    LIST_ENUMERATE(variant_lists, variants, i)
      transits[i] = SNetStreamCreate(0);
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      collstreams[i] = (*fun)(transits[i], info, location);
    END_ENUMERATE


    /* create parallel */
    parg = SNetMemAlloc( sizeof(parallel_arg_t));
    parg->input   = instream;
    /* copy */
    parg->outputs = transits;
    parg->variant_lists = variant_lists;
    parg->is_det = is_det;

    SNetEntitySpawn( ENTITY_PARALLEL, ParallelBoxTask, (void*)parg );


    /* create collector with collstreams */
    outstream = CollectorCreateStatic(num, collstreams, info);

    /* free collstreams array */
    SNetMemFree(collstreams);


  } else {
    LIST_ENUMERATE(variant_lists, variants, i)
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      instream = (*fun)( instream, info, location);
    END_ENUMERATE

    SNetVariantListListDestroy( variant_lists);

    outstream = instream;
  }

  SNetLocvecParallelLeave(locvec);

  SNetMemFree( funs);
  return( outstream);
}




/**
 * Parallel creation function
 */
snet_stream_t *SNetParallel( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...)
{
  va_list args;
  int i, num;
  void **funs;

  num = SNetVariantListListLength( variant_lists);
  funs = SNetMemAlloc( num * sizeof( void*));
  va_start( args, variant_lists);
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return CreateParallel( instream, info, location, variant_lists, funs, false);
}


/**
 * Det Parallel creation function
 */
snet_stream_t *SNetParallelDet( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...)
{
  va_list args;
  int i, num;
  void **funs;

  num = SNetVariantListListLength( variant_lists);
  funs = SNetMemAlloc( num * sizeof( void*));
  va_start( args, variant_lists);
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return CreateParallel( inbuf, info, location, variant_lists, funs, true);
}
