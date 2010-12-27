#include <assert.h>

#include "snetentities.h"

#include "record_p.h"
#include "typeencode.h"
#include "expression.h"
#include "memfun.h"
#include "collector.h"

#include "spawn.h"
#include "lpel.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#include "fun.h"
#include "debug.h"
#endif /* DISTRIBUTED_SNET */


static bool MatchesExitPattern( snet_record_t *rec, 
    snet_typeencoding_t *patterns, snet_expr_list_t *guards) 
{
  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    is_match = true;

    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( patterns, i+1);
      is_match = SNetRecPatternMatches(pat, rec);
    }
    else {
      is_match = false;
    }
    if( is_match) {
      break;
    }
  }
  return( is_match);
}



typedef struct {
  lpel_stream_t *input, *output;
  snet_typeencoding_t *exit_tags;
  snet_startup_fun_t box, selffun;
  snet_expr_list_t *guards;
  bool is_det, is_incarnate;
} star_arg_t;

/**
 * Star component task
 */
static void StarBoxTask( lpel_task_t *self, void *arg)
{
  star_arg_t *sarg = (star_arg_t *)arg;
  lpel_stream_desc_t *instream;
  lpel_stream_desc_t *outstream; /* the stream to the collector */
  /* The stream to the next instance;
     a non-null value indicates that the instance has been created. */
  lpel_stream_desc_t *nextstream=NULL;
  bool terminate = false;
  snet_record_t *rec;
  /* for deterministic variant: */
  int counter = 0;

#ifdef DISTRIBUTED_SNET
  int node_id;
  snet_fun_id_t fun_id;
#endif /* DISTRIBUTED_SNET */

  instream  = LpelStreamOpen( self, sarg->input, 'r');
  outstream = LpelStreamOpen( self, sarg->output, 'w');

#ifdef DISTRIBUTED_SNET
  node_id = SNetIDServiceGetNodeID();
  if(!SNetDistFunFun2ID(sarg->box, &fun_id)) {
    /* TODO: This is an error!*/
  }
#endif /* DISTRIBUTED_SNET */

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = LpelStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        if( MatchesExitPattern( rec, sarg->exit_tags, sarg->guards)) {
          /* send rec to collector */
          LpelStreamWrite( outstream, rec);
        } else {
          /* send to next instance */
          /* if instance has not been created yet, create it */
          if( nextstream == NULL) {
            /* The outstream to the collector from a newly created incarnate.
               Nothing is written to this stream, but it has to be registered
               at the collector. */
            snet_stream_t *starstream, *nextstream_addr;
            /* Create the stream to the instance */
            nextstream_addr = (snet_stream_t*) LpelStreamCreate();
            nextstream = LpelStreamOpen( self, (lpel_stream_t*) nextstream_addr, 'w');
            /* register new buffer with dispatcher,
               starstream is returned by selffun, which is SNetStarIncarnate */
            {
              snet_info_t *info;
              info = SNetInfoInit();
#ifdef DISTRIBUTED_SNET
              SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, node_id));
              starstream = SNetSerial(nextstream_addr, info, node_id , sarg->box, sarg->selffun);
              starstream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), starstream);
#else
              /* starstream is the outstream of BOX + STAR_INC serial */
              starstream = SNetSerial(nextstream_addr, info, sarg->box, sarg->selffun);
#endif /* DISTRIBUTED_SNET */
              SNetInfoDestroy(info);
            }
            /* notify collector about the new instance */
            LpelStreamWrite( outstream, SNetRecCreate(REC_collect, starstream));
          }
          /* send the record to the instance */
          LpelStreamWrite( nextstream, rec);
        } /* end if not matches exit pattern */

        /* deterministic non-incarnate has to append control records */
        if (sarg->is_det && !sarg->is_incarnate) {
          /* send new sort record to collector */
          LpelStreamWrite( outstream,
              SNetRecCreate( REC_sort_end, 0, counter) );

          /* if has next instance, send new sort record */
          if (nextstream != NULL) {
            LpelStreamWrite( nextstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
          }
        }
        break;

      case REC_sync:
        {
          lpel_stream_t *newstream = (lpel_stream_t*) SNetRecGetStream( rec);
          LpelStreamReplace( instream, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        assert(0);
        /* if ignoring, at least destroy ... */
        SNetRecDestroy( rec);
        break;

      case REC_sort_end:
        {
          int rec_lvl = SNetRecGetLevel(rec);
          /* send a copy to the box, if exists */
          if( nextstream != NULL) {
            LpelStreamWrite(
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
          LpelStreamWrite( outstream, rec);
        }
        break;

      case REC_terminate:
        terminate = true;
        if( nextstream != NULL) {
          LpelStreamWrite( nextstream, SNetRecCopy( rec));
        }
        LpelStreamWrite( outstream, rec);
        /* note that no sort record has to be appended */
        break;

      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  LpelStreamClose(instream, true);

  if( nextstream != NULL) {
    LpelStreamClose( nextstream, false);
  }
  LpelStreamClose( outstream, false);

  /* destroy the arg */
  SNetEdestroyList( sarg->guards);
  SNetDestroyTypeEncoding( sarg->exit_tags);
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
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b,
    bool is_incarnate,
    bool is_det
    )
{
  snet_stream_t *output;
  star_arg_t *sarg;
  lpel_stream_t *newstream;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    if (!is_incarnate) {
      SNetUtilDebugNotice( (is_det) ? "DetStar created" : "Star created");
    } else {
      SNetUtilDebugNotice((is_det)? "DetStar incarnate created" : "Star incarnate created");
    }
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    sarg->input = (lpel_stream_t*) input;
    newstream = LpelStreamCreate();
    if (!is_incarnate) {
      /* the "top-level" star also creates a collector */
      snet_stream_t **star_output;
      star_output = (snet_stream_t **) SNetMemAlloc( sizeof(lpel_stream_t *));
      star_output[0] = (snet_stream_t*) newstream;
      output = CollectorCreate( 1, star_output, info);
      sarg->output = newstream;
    } else {
      output = (snet_stream_t*) newstream;
      sarg->output = newstream;
    }
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_tags = type;
    sarg->guards = guards;
    sarg->is_incarnate = is_incarnate;
    sarg->is_det = is_det;

    SNetSpawnEntity( StarBoxTask, (void*)sarg,
        (is_det) ? ENTITY_star_det : ENTITY_star_nondet
        );

#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    output = input;
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}







/**
 * Star creation function
 */
snet_stream_t *SNetStar( snet_stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      type, guards, box_a, box_b,
      false, /* not incarnate */
      false /* not det */
      );
}



/**
 * Star incarnate creation function
 */
snet_stream_t *SNetStarIncarnate( snet_stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      type, guards, box_a, box_b,
      true, /* is incarnate */
      false /* not det */
      );
}



/**
 * Det Star creation function
 */
snet_stream_t *SNetStarDet(snet_stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      type, guards, box_a, box_b,
      false, /* not incarnate */
      true /* is det */
      );
}



/**
 * Det star incarnate creation function
 */
snet_stream_t *SNetStarDetIncarnate(snet_stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  return CreateStar( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      type, guards, box_a, box_b,
      true, /* is incarnate */
      true /* is det */
      );
}

