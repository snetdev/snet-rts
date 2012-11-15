#include <assert.h>

#include "snetentities.h"

#include "expression.h"
#include "memfun.h"
#include "locvec.h"
#include "collector.h"
#include "debug.h"

#include "threading.h"
#include "distribution.h"



extern int SNetLocvecTopval(snet_locvec_t *locvec);
//#define DEBUG_PRINT_GC


#define ENABLE_GARBAGE_COLLECTOR

/**
 * argument of the star function
 */
typedef struct {
  snet_info_t *info;
  snet_stream_t *input, *output;
  snet_variant_list_t *exit_patterns;
  snet_startup_fun_t box, selffun;
  snet_expr_list_t *guards;
  bool is_det, is_incarnate;
  int location;
} star_arg_t;




static bool MatchesExitPattern( snet_record_t *rec,
    snet_variant_list_t *exit_patterns, snet_expr_list_t *guards)
{
  int i;
  snet_variant_t *pattern;

  LIST_ENUMERATE(exit_patterns, i, pattern) {
    if (SNetRecPatternMatches(pattern, rec) &&
        SNetEevaluateBool( SNetExprListGet( guards, i), rec)) {
      return true;
    }
  }

  return false;
}


/**
 * A custom creation function for instances of star operands
 * - having a separate function here allows for distinction
 *   of an "ordinary" serial combinator and
 *   another replica of the serial replicator (star)
 */
static snet_stream_t *SNetSerialStarchild(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  snet_stream_t *internal_stream;
  snet_stream_t *output;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  (void) SNetLocvecStarSpawn(locvec);

  /* create operand A */
  SNetRouteDynamicEnter(info, SNetLocvecTopval(SNetLocvecGet(info)),
                        location, box_a);
  internal_stream = (*box_a)(input, info, location);
  internal_stream = SNetRouteUpdate(info, internal_stream, location);
  SNetRouteDynamicExit(info, SNetLocvecTopval(SNetLocvecGet(info)),
                       location, box_a);

  assert( SNetLocvecStarWithin(SNetLocvecGet(info)) );

  /* create operand B */
  output = (*box_b)(internal_stream, info, location);

  (void) SNetLocvecStarSpawnRet(locvec);

  return(output);
}


/**
 * Create the instance of the operand network, including
 * routing component (star incarnate) at the end
 */
static void CreateOperandNetwork(snet_stream_desc_t **next,
    star_arg_t *sarg, snet_stream_desc_t *to_coll)
{
  /* info structure for network creation */
  /* The outstream to the collector from a newly created incarnate.
     Nothing is written to this stream, but it has to be registered
     at the collector. */
  snet_stream_t *starstream, *nextstream_addr;
  /* Create the stream to the instance */
  nextstream_addr = SNetStreamCreate(0);

  /* Set the source of the stream to support garbage collection */
  SNetStreamSetSource(nextstream_addr, SNetLocvecGet(sarg->info));

  /* open the stream for the caller */
  *next = SNetStreamOpen(nextstream_addr, 'w');

  /* use custom creation function for proper/easier update of locvec */
  starstream = SNetSerialStarchild(
      nextstream_addr,
      sarg->info,
      sarg->location,
      sarg->box,
      sarg->selffun
      );

  /* notify collector about the new instance */
  SNetStreamWrite( to_coll, SNetRecCreate(REC_collect, starstream));
}




/**
 * Star component task
 */
static void StarBoxTask(snet_entity_t *ent, void *arg)
{
  star_arg_t *sarg = (star_arg_t *)arg;
  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstream; /* the stream to the collector */
  /* The stream to the next instance;
     a non-null value indicates that the instance has been created. */
  snet_stream_desc_t *nextstream=NULL;
  bool terminate = false;
  bool sync_cleanup = false;
  snet_record_t *rec;
  int counter = 0;
  (void) ent; /* NOT USED */

  instream  = SNetStreamOpen(sarg->input, 'r');
  outstream = SNetStreamOpen(sarg->output, 'w');

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);
    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        if( MatchesExitPattern( rec, sarg->exit_patterns, sarg->guards)) {
          assert(!sync_cleanup);
#ifdef DEBUG_PRINT_GC
          SNetUtilDebugNoticeEnt( ent,
              "[STAR] Notice: Data leaves replication network.");
#endif
          /* send rec to collector */
          SNetStreamWrite( outstream, rec);
        } else {
          /* if instance has not been created yet, create it */
          if( nextstream == NULL) {
            CreateOperandNetwork(&nextstream, sarg, outstream);
          }
          /* send the record to the instance */
          SNetStreamWrite( nextstream, rec);
        } /* end if not matches exit pattern */

        /* deterministic non-incarnate has to append control records */
        if (sarg->is_det && !sarg->is_incarnate) {
          /* send new sort record to collector level=0, counter=0*/
          SNetStreamWrite( outstream,
              SNetRecCreate( REC_sort_end, 0, counter) );

          /* if has next instance, send new sort record */
          if (nextstream != NULL) {
            SNetStreamWrite( nextstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
          }
          /* increment counter */
          counter++;

        }
#ifdef ENABLE_GARBAGE_COLLECTOR
        else if (sync_cleanup) {
          snet_record_t *term_rec;
          /*
           * If sync_cleanup is set, we decided to postpone termination
           * due to garbage collection triggered by a sync record until now.
           * Postponing was done in order not to create the operand network unnecessarily
           * only to be able to forward the sync record.
           */
          assert( nextstream != NULL);
          /* first send a sync record to the next instance */
          SNetStreamWrite( nextstream,
              SNetRecCreate( REC_sync, sarg->input) );


          /* send a terminate record to collector, it will close and
             destroy the stream */
          term_rec = SNetRecCreate(REC_terminate);
          SNetRecSetFlag(term_rec);
          SNetStreamWrite( outstream, term_rec);

          terminate = true;
#ifdef DEBUG_PRINT_GC
          /* terminating due to GC */
          SNetUtilDebugNoticeEnt( ent,
              "[STAR] Notice: Destroying star dispatcher due to GC, "
              "delayed until new data record!"
              );
#endif
          SNetStreamClose(nextstream, false);
          SNetStreamClose(instream, false);
        }
#endif /* ENABLE_GARBAGE_COLLECTOR */
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream( rec);
#ifdef ENABLE_GARBAGE_COLLECTOR
          snet_locvec_t *loc = SNetStreamGetSource( newstream);
#ifdef DEBUG_PRINT_GC
          if (loc != NULL) {
            char srecloc[64];
            SNetLocvecPrint(srecloc, 64, loc);
            SNetUtilDebugNoticeEnt( ent,
                  "[STAR] Notice: Received sync record with a stream with source %s.",
                  srecloc
                  );
          }
#endif
          /* TODO
           * It is not necessary to carry the whole location vector in the
           * next stream of a star-entity, only a flag. As a prerequisite,
           * non_incarnates must not clean themselves up!
           */
          /*
           * Only incarnates are eligible for cleanup!
           * check if the source (location) of the stream and the own location are
           * (subsequent) star dispatcher entities of the same star combinator network
           * -> if so, we can clean-up ourselves
           */
          if ( sarg->is_incarnate && loc != NULL ) {
            assert( true == SNetLocvecEqualParent(loc, SNetLocvecGet(sarg->info)) );
            /* If the next instance is already created, we can forward the sync-record
             * immediately and terminate.
             * Otherwise we postpone termination to the point when a next data record
             * is received, as we create the operand network then.
             */
            if (nextstream != NULL) {
              snet_record_t *term_rec;
              /* forward the sync record  */
              SNetStreamWrite( nextstream, rec);
              /* send a terminate record to collector, it will close and
                 destroy the stream */
              term_rec = SNetRecCreate(REC_terminate);
              SNetRecSetFlag(term_rec);
              SNetStreamWrite( outstream, term_rec);

              terminate = true;
#ifdef DEBUG_PRINT_GC
              /* terminating due to GC */
              SNetUtilDebugNoticeEnt( ent,
                  "[STAR] Notice: Destroying star dispatcher due to GC, "
                  "immediately on sync!"
                  );
#endif
              SNetStreamClose(nextstream, false);
              SNetStreamClose(instream, true);

            } else {
              sync_cleanup = true;
#ifdef DEBUG_PRINT_GC
              SNetUtilDebugNoticeEnt( ent,
                  "[STAR] Notice: Remembering delayed destruction.");
#endif
              /* handle sync record as usual */
              SNetStreamReplace( instream, newstream);
              sarg->input = newstream;
              SNetRecDestroy( rec);
            }
          } else
#endif /* ENABLE_GARBAGE_COLLECTOR */
          {
            /* handle sync record as usual */
            SNetStreamReplace( instream, newstream);
            sarg->input = newstream;
            SNetRecDestroy( rec);
          }
        }
        break;


      case REC_sort_end:
        {
          int rec_lvl = SNetRecGetLevel(rec);
          /* send a copy to the box, if exists */
          if( nextstream != NULL) {
            SNetStreamWrite(
                nextstream,
                SNetRecCreate( REC_sort_end,
                  (!sarg->is_incarnate)? rec_lvl+1 : rec_lvl,
                  SNetRecGetNum(rec) )
                );
          }

          /* send the original one to the collector */
          if (!sarg->is_incarnate) {
            /* if non-incarnate, we have to increase level */
            SNetRecSetLevel( rec, rec_lvl+1);
          }
          SNetStreamWrite( outstream, rec);
        }
        break;

      case REC_terminate:
        terminate = true;
        if( nextstream != NULL) {
          SNetStreamWrite( nextstream, SNetRecCopy( rec));
          SNetStreamClose( nextstream, false);
        }
        SNetStreamWrite( outstream, rec);
        /* note that no sort record has to be appended */
        SNetStreamClose(instream, true);
        break;

      case REC_collect:
      default:
        SNetUtilDebugFatal("Unknown record type!");
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */


  /* close streams */
  SNetStreamClose( outstream, false);

  /* destroy the task argument */
  SNetExprListDestroy( sarg->guards);
  SNetLocvecDestroy(SNetLocvecGet(sarg->info));
  SNetInfoDestroy(sarg->info);
  SNetVariantListDestroy( sarg->exit_patterns);
  SNetMemFree( sarg);
} /* STAR BOX TASK END */




/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/


/**
 * Convenience function for creating
 * Star, DetStar, StarIncarnate or DetStarIncarnate,
 * dependent on parameters is_incarnate and is_det
 */
static snet_stream_t *CreateStar( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b,
    bool is_incarnate,
    bool is_det
    )
{
  snet_stream_t *output;
  star_arg_t *sarg;
  snet_stream_t *newstream;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  if (!is_incarnate) {
    SNetLocvecStarEnter(locvec);
    input = SNetRouteUpdate(info, input, location);
  } else {
    input = SNetRouteUpdate(info, input, location);
  }

  if(SNetDistribIsNodeLocation(location)) {
    /* create the task argument */
    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    newstream = SNetStreamCreate(0);
    sarg->input = input;
    /* copy location vector */
    sarg->output = newstream;
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_patterns = exit_patterns;
    sarg->guards = guards;
    sarg->info = SNetInfoCopy(info);
    SNetLocvecSet(sarg->info, SNetLocvecCopy(locvec));
    sarg->is_incarnate = is_incarnate;
    sarg->is_det = is_det;
    sarg->location = location;

    SNetThreadingSpawn(
        SNetEntityCreate( ENTITY_star, location, locvec,
          "<star>", StarBoxTask, (void*)sarg)
        );

    /* creation function of top level star will return output stream
     * of its collector, the incarnates return their outstream
     */
    if (!is_incarnate) {
      /* the "top-level" star also creates a collector */
      output = CollectorCreateDynamic(newstream, location, info);
    } else {
      output = newstream;
    }

  } else {
    SNetExprListDestroy( guards);
    SNetVariantListDestroy(exit_patterns);
    output = input;
  }

  if (!is_incarnate) SNetLocvecStarLeave(SNetLocvecGet(info));

  return( output);
}




/**
 * Star creation function
 */
snet_stream_t *SNetStar( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
      false, /* not incarnate */
      false /* not det */
      );
}



/**
 * Star incarnate creation function
 */
snet_stream_t *SNetStarIncarnate( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
      true, /* is incarnate */
      false /* not det */
      );
}



/**
 * Det Star creation function
 */
snet_stream_t *SNetStarDet(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
      false, /* not incarnate */
      true /* is det */
      );
}



/**
 * Det star incarnate creation function
 */
snet_stream_t *SNetStarDetIncarnate(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
      true, /* is incarnate */
      true /* is det */
      );
}
