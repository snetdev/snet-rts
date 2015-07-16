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
#include "limits.h"

//#define DEBUG_PRINT_GC

#define ENABLE_GARBAGE_COLLECTOR

typedef struct {
  snet_stream_t *input;
  snet_stream_t **outputs;
  snet_variant_list_list_t *variant_lists;
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
  int name;
  snet_variant_t *variant;
  int max=-1;
  int val;
  (void) val; /* NOT USED */

  assert(rec != NULL);
  assert(variant_list != NULL);
  assert(mc != NULL);

  /* for all variants */
  LIST_FOR_EACH(variant_list, variant) {
    mc->count = 0;
    mc->is_match = true;

    VARIANT_FOR_EACH_FIELD(variant, name) {
      MatchCountUpdate(mc, SNetRecHasField(rec, name));
    }

    VARIANT_FOR_EACH_TAG(variant, name) {
      MatchCountUpdate(mc, SNetRecHasTag(rec, name));
    }

    RECORD_FOR_EACH_BTAG(rec, name, val) {
      MatchCountUpdate(mc, SNetVariantHasBTag(variant, name));
    }

    if (mc->is_match) {
      max = mc->count > max ? mc->count : max;
    }
  }

  if( max >= 0) {
    mc->is_match = true;
    mc->count = max;
  }
}



#ifdef ENABLE_GARBAGE_COLLECTOR

static bool VariantIsSupertypeOfAllOthers(snet_variant_t *var,
    snet_variant_list_t *variant_list)
{
  snet_variant_t *other;
  int name;

  /* for all variants in the list */
  LIST_FOR_EACH(variant_list, other) {
    VARIANT_FOR_EACH_FIELD(var, name) {
      if (!SNetVariantHasField(other, name)) return false;
    }

    VARIANT_FOR_EACH_TAG(var, name) {
      if (!SNetVariantHasTag(other, name)) return false;
    }

    VARIANT_FOR_EACH_BTAG(var, name) {
      if (!SNetVariantHasBTag(other, name)) return false;
    }
  }
  return true;
}

#endif /* ENABLE_GARBAGE_COLLECTOR */


#if 0
/* This is no longer used and can therefore be removed ? */

/**
 * Using weighted round robin to decide which buffer to dispatch to:
 *   among all best match branches, choose the least-used one
 */
static int WeightedRoundRobin( match_count_t **counter, int *usedcounter, int num)
{
  int i, j;
  int max, min;
  int *best;
  int num_best;
  int res;

  max = -1;
  num_best = 0;
  best = SNetMemAlloc( num * sizeof(int));

  for( i=0; i<num; i++) {
    if( counter[i]->is_match) {
      if( counter[i] ->count > max) {
        best[0] = i;
        num_best = 1;
        max = counter[i]->count;
      }
      if( counter[i]->count == max) {
        best[num_best] = i;
        num_best += 1;
      }
    }
  }

  min = INT_MAX;
  res = -1;
  for( i=0; i<num_best; i++){
    j = best[i];
    if( usedcounter[j] < min) {
      res = j;
      min = usedcounter[j];
    }
  }
  SNetMemFree( best);
  return res;
}
#endif

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
static void ParallelBoxTask(snet_entity_t *ent, void *arg)
{
  parallel_arg_t *parg = (parallel_arg_t *) arg;
  /* the number of outputs */
  int num = SNetVariantListListLength(parg->variant_lists);
  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstreams[num];
  int i;
  snet_record_t *rec;
  match_count_t **matchcounter;
  int num_init_branches = 0;
  bool terminate = false;
  int counter = 0;
  int *usedcounter; /* to keep track how many times the branch is used */

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
      rec = SNetRecCreate( REC_terminate);
      SNetRecSetFlag(rec);
      PutToBuffers( outstreams, num, i, rec, NULL);
      SNetStreamClose( outstreams[i], false);
      outstreams[i] = NULL;
      num_init_branches += 1;
#ifdef DEBUG_PRINT_GC
      SNetUtilDebugNoticeEnt( ent,
          "[PAR] Terminate initialiser branch %d!", i);
#endif
    } else {
      /* on non-initialiser branches, send a sort record */
      SNetStreamWrite( outstreams[i], SNetRecCreate( REC_sort_end, 0, counter));
    }
  }
  counter++;

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
  usedcounter = SNetMemAlloc( num * sizeof( int));
  for( i=0; i<num; i++) {
    matchcounter[i] = SNetMemAlloc( sizeof( match_count_t));
    usedcounter[i] = 0;
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
          // used first match stategy
          stream_index = BestMatch( matchcounter, num);

         // use Weighted Round Robin strategy
         // stream_index = WeightedRoundRobin( matchcounter, usedcounter, num);

          if (stream_index == -1) {
            SNetUtilDebugFatalEnt( ent,
                "[PAR] Cannot route data record, no matching branch!");
          }
          PutToBuffers( outstreams, num, stream_index, rec, (parg->is_det)? &counter : NULL);
          usedcounter[stream_index] += 1;  // increase the number of usage

        }
        break;

      case REC_sync:
        {
#ifdef ENABLE_GARBAGE_COLLECTOR
          snet_variant_t *synctype = SNetRecGetVariant(rec);
          if (synctype != NULL) {
            snet_stream_desc_t *last = NULL;
            int cnt = 0;

            /* terminate affected branches */
            for( i=0; i<num; i++) {
              if ( VariantIsSupertypeOfAllOthers( synctype,
                    SNetVariantListListGet( parg->variant_lists, i)) ) {
                snet_record_t *term_rec = SNetRecCreate(REC_terminate);
#ifdef DEBUG_PRINT_GC
                SNetUtilDebugNoticeEnt( ent,
                    "[PAR] Terminate branch %d due to outtype of synchrocell!", i);
#endif
                SNetRecSetFlag(term_rec);
                SNetStreamWrite(outstreams[i], term_rec);
                SNetStreamClose(outstreams[i], false);
                outstreams[i] = NULL;
              }
            }

            /* count remaining branches */
            for (i=0;i<num;i++) {
            	if (outstreams[i] != NULL) {
            		cnt++;
            		last = outstreams[i];
            	}
            }
            counter++;

            /* if only one branch left, we can terminate ourselves*/
            if (cnt == 1) {

            	/* send a sort end to notify the paired collector so that the collector know all sort_end records coming from the last branch was not rasied the level by the this parallel entity
            	 * Special (l0, c-1) is used for this purpose
            	 * On the path the sort_end may reach other parallel and collector, parallel increases the level by 1, and collector decreases by 1
            	 * When the sort_end record reaches the relevant collector, its level should be zero
            	 * One collector is supposed to receive at most 1 this special sort_end record. After receiving it, the collector will terminate soon as well
            	 * */
            	snet_record_t *notify_rec = SNetRecCreate(REC_sort_end, 0, -1);
            	SNetStreamWrite(last, notify_rec);

              /* forward stripped sync record */
              SNetRecSetVariant(rec, NULL);
              SNetStreamWrite(last, rec);

              /* close instream */
              SNetStreamClose( instream, true);
              terminate = true;
#ifdef DEBUG_PRINT_GC
              SNetUtilDebugNoticeEnt( ent,
                  "[PAR] Terminate self, as only one branch left!");
#endif
            } else {
              /* usual sync replace */
              parg->input = SNetRecGetStream( rec);
              SNetStreamReplace( instream, parg->input);
              SNetRecDestroy( rec);
            }

          } else
#endif /* ENABLE_GARBAGE_COLLECTOR */
          {
            /* usual sync replace */
            parg->input = SNetRecGetStream( rec);
            SNetStreamReplace( instream, parg->input);
            SNetRecDestroy( rec);
          }
        }
        break;

      case REC_collect:
        SNetUtilDebugNoticeEnt( ent, "[PAR] Received REC_collect, destroying it");
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

      default:
        SNetUtilDebugNoticeEnt( ent, "[PAR] Unknown control rec destroyed (%d).\n",
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
  SNetMemFree( usedcounter);

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
    snet_startup_fun_t *funs, bool is_det)
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
  (void) variants; /* NOT USED */

  num = SNetVariantListListLength( variant_lists);

  locvec = SNetLocvecGet(info);
  SNetLocvecParallelEnter(locvec);

  instream = SNetRouteUpdate(info, instream, location);
  if(SNetDistribIsNodeLocation(location)) {
    transits    = SNetMemAlloc( num * sizeof( snet_stream_t*));
    collstreams = SNetMemAlloc( num * sizeof( snet_stream_t*));

    /* create branches */
    LIST_ENUMERATE(variant_lists, i, variants) {
      snet_info_t *newInfo = SNetInfoCopy(info);
      transits[i] = SNetStreamCreate(0);
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      collstreams[i] = (*fun)(transits[i], newInfo, location);
      collstreams[i] = SNetRouteUpdate(newInfo, collstreams[i], location);
      SNetInfoDestroy(newInfo);
    }

    SNetLocvecParallelReset(locvec);
    /* create parallel */
    parg = SNetMemAlloc( sizeof(parallel_arg_t));
    parg->input   = instream;
    /* copy */
    parg->outputs = transits;
    parg->variant_lists = variant_lists;
    parg->is_det = is_det;

    SNetThreadingSpawn(
        SNetEntityCreate( ENTITY_parallel, location, locvec,
          "<parallel>", ParallelBoxTask, (void*)parg)
        );

    /* create collector with collstreams */
    SNetLocvecEndBorder(locvec);		// set end border index for collector
    outstream = CollectorCreateStatic(num, collstreams, location, info);
    SNetLocvecResetBorder(locvec);

    /* free collstreams array */
    SNetMemFree(collstreams);


  } else {
    LIST_ENUMERATE(variant_lists, i, variants) {
      snet_info_t *newInfo = SNetInfoCopy(info);
      SNetLocvecParallelNext(locvec);
      fun = funs[i];
      instream = (*fun)( instream, newInfo, location);
      instream = SNetRouteUpdate(newInfo, instream, location);
      SNetInfoDestroy(newInfo);
    }

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
  snet_startup_fun_t *funs;

  num = SNetVariantListListLength( variant_lists);
  funs = SNetMemAlloc( num * sizeof( snet_startup_fun_t));
  va_start( args, variant_lists);
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, snet_startup_fun_t);
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
  snet_startup_fun_t *funs;

  num = SNetVariantListListLength( variant_lists);
  funs = SNetMemAlloc( num * sizeof( snet_startup_fun_t));
  va_start( args, variant_lists);
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, snet_startup_fun_t);
  }
  va_end( args);

  return CreateParallel( inbuf, info, location, variant_lists, funs, true);
}
