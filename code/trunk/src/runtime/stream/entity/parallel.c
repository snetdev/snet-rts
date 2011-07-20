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

#include <assert.h>

#include "snetentities.h"
#include "collector.h"
#include "memfun.h"
#include "debug.h"
#include "locvec.h"
#include "distribution.h"
#include "threading.h"

//#define DEBUG_PRINT_GC


typedef struct {
  snet_stream_t *input;
  snet_stream_t **outputs;
  snet_variant_list_list_t *variant_lists;
  snet_locvec_t *myloc;
  bool is_det;
} parallel_arg_t;

typedef struct {
  bool is_match;
  int count;
} match_count_t;



static inline void MatchCountUpdate( match_count_t *mc, bool match_cond)
{
  if (match_cond) {
    mc->count += 1;
  } else {
    mc->is_match = false;
  }
}


static void CheckMatch( snet_record_t *rec,
    snet_variant_list_t *variant_list, match_count_t *mc)
{
  int name, val;
  snet_variant_t *variant;
  int max=-1;

  assert(rec != NULL);
  assert(variant_list != NULL);
  assert(mc != NULL);

  /* for all variants */
  LIST_FOR_EACH(variant_list, variant)
    mc->count = 0;
    mc->is_match = true;

    VARIANT_FOR_EACH_FIELD(variant, name)
      MatchCountUpdate(mc, SNetRecHasField(rec, name));
    END_FOR

    VARIANT_FOR_EACH_TAG(variant, name)
      MatchCountUpdate(mc, SNetRecHasTag(rec, name));
    END_FOR

    RECORD_FOR_EACH_BTAG(rec, name, val)
      MatchCountUpdate(mc, SNetVariantHasBTag(variant, name));
    END_FOR

    if (mc->is_match) {
      max = mc->count > max ? mc->count : max;
    }
  END_FOR

  if( max >= 0) {
    mc->is_match = true;
    mc->count = max;
  }
}


static bool VariantIsSupertypeOfAllOthers(snet_variant_t *var,
    snet_variant_list_t *variant_list)
{
  snet_variant_t *other;
  int name;

  /* for all variants in the list */
  LIST_FOR_EACH(variant_list, other)
    VARIANT_FOR_EACH_FIELD(var, name)
      if (!SNetVariantHasField(other, name)) return false;
    END_FOR

    VARIANT_FOR_EACH_TAG(var, name)
      if (!SNetVariantHasTag(other, name)) return false;
    END_FOR

    VARIANT_FOR_EACH_BTAG(var, name)
      if (!SNetVariantHasBTag(other, name)) return false;
    END_FOR
  END_FOR
  return true;
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
    if( counter[i]->is_match) {
      if( counter[i]->count > max) {
        res = i;
        max = counter[i]->count;
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
  int i;
  snet_record_t *rec;
  snet_record_t *sourcerec = NULL;
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
    /* read a record from the instream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        {
          int stream_index;
          for( i=0; i<num; i++) {
            CheckMatch( rec, SNetVariantListListGet( parg->variant_lists, i), matchcounter[i]);
          }
          stream_index = BestMatch( matchcounter, num);
          if (stream_index == -1) {
            SNetUtilDebugNoticeLoc( parg->myloc,
                "[PAR] Cannot route data record, no matching branch!");
          }
          PutToBuffers( outstreams, num, stream_index, rec, (parg->is_det)? &counter : NULL);
        }
        break;

      case REC_sync:
        {
          snet_variant_t *synctype = SNetRecGetVariant(rec);
          if (synctype!=NULL && sourcerec!=NULL) {
            snet_stream_desc_t *last;
            int cnt = 0;
            /* terminate affected branches */
            for( i=0; i<num; i++) {
              if ( VariantIsSupertypeOfAllOthers( synctype,
                    SNetVariantListListGet( parg->variant_lists, i)) ) {
#ifdef DEBUG_PRINT_GC
                SNetUtilDebugNoticeLoc( parg->myloc,
                    "[PAR] Terminate branch %d due to outtype of synchrocell!", i);
#endif
                SNetStreamWrite(outstreams[i], SNetRecCreate(REC_terminate));
                SNetStreamClose(outstreams[i], false);
                outstreams[i] = NULL;
              }
            }



            /* count remaining branches, and send a sort record through */
            for (i=0;i<num;i++) {
              if (outstreams[i] != NULL) {
                cnt++;
                last = outstreams[i];
                SNetStreamWrite( last,
                    SNetRecCreate( REC_sort_end, 0, counter));
              }
              counter += 1;
            }

            /* if only one branch left, we can terminate ourselves*/
            if (cnt == 1) {
              /* forward source record */
              SNetStreamWrite(last, sourcerec);
              sourcerec = NULL;

              /* forward stripped sync record */
              SNetRecSetVariant(rec, NULL);
              SNetStreamWrite(last, rec);

              /* close instream */
              SNetStreamClose( instream, false);
              terminate = true;
              // FIXME FIXME FIXME
#ifdef DEBUG_PRINT_GC
              SNetUtilDebugNoticeLoc( parg->myloc,
                  "[PAR] Terminate self, as only one branch left!");
#endif
            } else {
              /* usual sync replace */
              parg->input = SNetRecGetStream( rec);
              SNetStreamReplace( instream, parg->input);
              SNetRecDestroy( rec);
            }

          } else {
            /* usual sync replace */
            parg->input = SNetRecGetStream( rec);
            SNetStreamReplace( instream, parg->input);
            SNetRecDestroy( rec);
          }
          /* in either case, clear the stored source record! */
          if (sourcerec != NULL) {
            SNetRecDestroy(sourcerec);
            sourcerec = NULL;
          }
        }
        break;

      case REC_collect:
        SNetUtilDebugNoticeLoc( parg->myloc, "[PAR] Received REC_collect, destroying it");
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
        /* close and destroy instream */
        SNetStreamClose( instream, true);
        /* note that no sort record needs to be appended */
        break;

      case REC_source:
        /* store temporarily */
        assert(sourcerec == NULL);
        sourcerec = rec;
        break;

      default:
        SNetUtilDebugNoticeLoc(parg->myloc, "[PAR] Unknown control rec destroyed (%d).\n",
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

  SNetMemFree(parg->myloc);

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
  if(SNetDistribIsNodeLocation(location)) {
    transits    = SNetMemAlloc( num * sizeof( snet_stream_t*));
    collstreams = SNetMemAlloc( num * sizeof( snet_stream_t*));

    /* create branches */
    LIST_ENUMERATE(variant_lists, variants, i)
      snet_info_t *newInfo = SNetInfoCopy(info);
      transits[i] = SNetStreamCreate(0);
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      collstreams[i] = (*fun)(transits[i], newInfo, location);
      collstreams[i] = SNetRouteUpdate(newInfo, collstreams[i], location);
      SNetInfoDestroy(newInfo);
    END_ENUMERATE

    SNetLocvecParallelReset(locvec);

    /* create parallel */
    parg = SNetMemAlloc( sizeof(parallel_arg_t));
    parg->input   = instream;
    /* copy */
    parg->outputs = transits;
    parg->variant_lists = variant_lists;
    parg->myloc = SNetLocvecCopy(locvec);
    parg->is_det = is_det;

    SNetEntitySpawn( ENTITY_parallel, locvec, location,
        "<parallel>", ParallelBoxTask, (void*)parg);

    /* create collector with collstreams */
    outstream = CollectorCreateStatic(num, collstreams, location, info);

    /* free collstreams array */
    SNetMemFree(collstreams);


  } else {
    LIST_ENUMERATE(variant_lists, variants, i)
      snet_info_t *newInfo = SNetInfoCopy(info);
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      instream = (*fun)( instream, newInfo, location);
      instream = SNetRouteUpdate(newInfo, instream, location);
      SNetInfoDestroy(newInfo);
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
