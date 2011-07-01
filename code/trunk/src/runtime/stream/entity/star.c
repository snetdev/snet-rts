#include <assert.h>

#include "snetentities.h"

#include "expression.h"
#include "memfun.h"
#include "locvec.h"
#include "collector.h"

#include "threading.h"
#include "distribution.h"


/**
 * argument of the star function
 */
typedef struct {
  snet_stream_t *input, *output;
  snet_variant_list_t *exit_patterns;
  snet_startup_fun_t box, selffun;
  snet_expr_list_t *guards;
  snet_locvec_t *locvec;
  bool is_det, is_incarnate;
  int location;
} star_arg_t;




static bool MatchesExitPattern( snet_record_t *rec,
    snet_variant_list_t *exit_patterns, snet_expr_list_t *guards)
{
  int i;
  snet_variant_t *pattern;

  LIST_ENUMERATE(exit_patterns, pattern, i)
    if( SNetEevaluateBool( SNetExprListGet( guards, i), rec) &&
        SNetRecPatternMatches(pattern, rec)) {
      return true;
    }
  END_ENUMERATE

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

  SNetRouteUpdate(info, input, location);


  /* create operand A */
  internal_stream = (*box_a)(input, info, location);

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
  snet_info_t *info = SNetInfoInit();
  SNetLocvecSet(info, sarg->locvec);
  /* The outstream to the collector from a newly created incarnate.
     Nothing is written to this stream, but it has to be registered
     at the collector. */
  snet_stream_t *starstream, *nextstream_addr;
  /* Create the stream to the instance */
  nextstream_addr = SNetStreamCreate(0);
  *next = SNetStreamOpen(nextstream_addr, 'w');

  /* send a REC_source to nextstream, for garbage collection */
  SNetStreamWrite( *next, SNetRecCreate(REC_source, sarg->locvec));

  /* register new buffer with dispatcher,
     starstream is returned by selffun, which is SNetStarIncarnate */
  /* use custom creation function for proper/easier update of locvec */
  starstream = SNetSerialStarchild(
      nextstream_addr,
      info,
      sarg->location,
      sarg->box,
      sarg->selffun
      );

  SNetInfoDestroy( info);

  /* notify collector about the new instance */
  SNetStreamWrite( to_coll, SNetRecCreate(REC_collect, starstream));
}




/**
 * Star component task
 */
static void StarBoxTask(void *arg)
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
              SNetRecCreate( REC_sort_end, 0, 0) );

          /* if has next instance, send new sort record */
          if (nextstream != NULL) {
            SNetStreamWrite( nextstream,
                SNetRecCreate( REC_sort_end, 0, 0) );
          }
        } else if (sync_cleanup) {
          /* If sync_cleanup is set, then we received a source record that
           * told us we will receive a REC_sync next containing the
           * outstream of a predecessor star dispatcher.
           * The sync rec immediately followed the source rec.
           * Cleaning up is done at the next incoming  data record (now)
           * such that the operand network is not created unnecessarily
           * only to be able to forward the sync record.
           */
          assert( nextstream != NULL);
          /* first send a sync record to the next instance */
          SNetStreamWrite( nextstream,
              SNetRecCreate( REC_sync, sarg->input) );

          /* send a terminate record to collector, it will close and
             destroy the stream */
          SNetStreamWrite( outstream, SNetRecCreate(REC_terminate));

          terminate = true;
          SNetStreamClose(nextstream, false);
          SNetStreamClose(instream, false);
        }
        break;

      case REC_sync:
        if (sync_cleanup && nextstream != NULL) {
          /* If sync_cleanup is set, then we received a source record that
           * told us we will receive a REC_sync next containing the
           * outstream of a predecessor star dispatcher.
           * Here it is, to avoid stale star dispatcher components, we will
           * clean ourselves up and forward the REC_sync to the operand network
           */
          /* forward the sync record  */
          SNetStreamWrite( nextstream, rec);
          /* send a terminate record to collector, it will close and
             destroy the stream */
          SNetStreamWrite( outstream, SNetRecCreate(REC_terminate));

          terminate = true;
          SNetStreamClose(nextstream, false);
          SNetStreamClose(instream, false);
        } else {
          /* handle sync record as usual */
          snet_stream_t *newstream = SNetRecGetStream( rec);
          SNetStreamReplace( instream, newstream);
          sarg->input = newstream;
          SNetRecDestroy( rec);
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

      case REC_source:
        {
          snet_locvec_t *loc = SNetRecGetLocvec( rec);
          assert( loc != NULL );
          /* check if sarg->loc and the received location are
           * star dispatcher instances of the same star combinator network
           * -> if so, we can clean-up ourselves on next sync
           */
          if (SNetLocvecEqualParent(sarg->locvec, loc)) {
            sync_cleanup = true;
          }
          /* destroy */
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */


  /* close streams */
  SNetStreamClose( outstream, false);

  /* destroy the task argument */
  SNetExprListDestroy( sarg->guards);
  SNetLocvecDestroy( sarg->locvec );
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
  if (!is_incarnate) SNetLocvecStarEnter(locvec);

  input = SNetRouteUpdate(info, input, location);
  if(SNetDistribIsNodeLocation(location)) {
    /* create the task argument */
    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    newstream = SNetStreamCreate(0);
    sarg->input = input;
    /* copy location vector */
    sarg->locvec = SNetLocvecCopy(locvec);
    sarg->output = newstream;
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_patterns = exit_patterns;
    sarg->guards = guards;
    sarg->is_incarnate = is_incarnate;
    sarg->is_det = is_det;
    sarg->location = location;

    SNetEntitySpawn( ENTITY_star, sarg->locvec, location,
        "<star>", StarBoxTask, (void*)sarg);

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

  if (!is_incarnate) SNetLocvecStarLeave(locvec);

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
