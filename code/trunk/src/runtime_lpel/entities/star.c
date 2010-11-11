#include <assert.h>

#include "snetentities.h"

#include "record_p.h"
#include "typeencode.h"
#include "threading.h"
#include "expression.h"
#include "memfun.h"
#include "collector.h"

#include "stream.h"
#include "task.h"

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
  stream_t *input, *output;
  snet_typeencoding_t *exit_tags;
  snet_startup_fun_t box, selffun;
  snet_expr_list_t *guards;
  bool is_det, is_incarnate;
} star_arg_t;

/**
 * Star component task
 */
static void StarBoxTask( task_t *self, void *arg)
{
  star_arg_t *sarg = (star_arg_t *)arg;
  stream_desc_t *instream;
  stream_desc_t *outstream; /* the stream to the collector */
  /* The stream to the next instance;
     a non-null value indicates that the instance has been created. */
  stream_desc_t *nextstream=NULL;
  bool terminate = false;
  snet_record_t *rec;
  /* for deterministic variant: */
  int counter = 0;

#ifdef DISTRIBUTED_SNET
  int node_id;
  snet_fun_id_t fun_id;
#endif /* DISTRIBUTED_SNET */

  instream  = StreamOpen( self, sarg->input, 'r');
  outstream = StreamOpen( self, sarg->output, 'w');

#ifdef DISTRIBUTED_SNET
  node_id = SNetIDServiceGetNodeID();
  if(!SNetDistFunFun2ID(sarg->box, &fun_id)) {
    /* TODO: This is an error!*/
  }
#endif /* DISTRIBUTED_SNET */

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = StreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        if( MatchesExitPattern( rec, sarg->exit_tags, sarg->guards)) {
          /* send rec to collector */
          StreamWrite( outstream, rec);
        } else {
          /* send to next instance */
          /* if instance has not been created yet, create it */
          if( nextstream == NULL) {
            /* The outstream to the collector from a newly created incarnate.
               Nothing is written to this stream, but it has to be registered
               at the collector. */
            stream_t *starstream, *nextstream_addr;
            /* Create the stream to the instance */
            nextstream_addr = StreamCreate();
            nextstream = StreamOpen( self, nextstream_addr, 'w');
            /* register new buffer with dispatcher,
               starstream is returned by selffun, which is SNetStarIncarnate */
#ifdef DISTRIBUTED_SNET
            {
              snet_info_t *info;
              info = SNetInfoInit();
              SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, node_id));
              starstream = SNetSerial(nextstream_addr, info, node_id , sarg->box, sarg->selffun);
              starstream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), starstream);
              SNetInfoDestroy(info);
            }
#else
            /* starstream is the outstream of BOX + STAR_INC serial */
            starstream = SNetSerial(nextstream_addr, sarg->box, sarg->selffun);
#endif /* DISTRIBUTED_SNET */
            /* notify collector about the new instance */
            StreamWrite( outstream, SNetRecCreate(REC_collect, starstream));
          }
          /* send the record to the instance */
          StreamWrite( nextstream, rec);
        } /* end if not matches exit pattern */

        /* deterministic non-incarnate has to append control records */
        if (sarg->is_det && !sarg->is_incarnate) {
          /* send new sort record to collector */
          StreamWrite( outstream,
              SNetRecCreate( REC_sort_end, 0, counter) );

          /* if has next instance, send new sort record */
          if (nextstream != NULL) {
            StreamWrite( nextstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
          }
        }
        break;

      case REC_sync:
        {
          stream_t *newstream = SNetRecGetStream( rec);
          StreamReplace( instream, newstream);
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
            StreamWrite(
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
          StreamWrite( outstream, rec);
        }
        break;

      case REC_terminate:
        terminate = true;
        if( nextstream != NULL) {
          StreamWrite( nextstream, SNetRecCopy( rec));
        }
        StreamWrite( outstream, rec);
        /* note that no sort record has to be appended */
        break;

      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  StreamClose(instream, true);

  if( nextstream != NULL) {
    StreamClose( nextstream, false);
  }
  StreamClose( outstream, false);

  /* destroy the arg */
  SNetEdestroyList( sarg->guards);
  SNetDestroyTypeEncoding( sarg->exit_tags);
  SNetMemFree( sarg);
} /* STAR BOX TASK END */



/* ------------------------------------------------------------------------- */
/*  SNetStar                                                                 */
/* ------------------------------------------------------------------------- */


/**
 * Star creation function
 */
stream_t *SNetStar( stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  stream_t **star_output, *dispatch_output;
  star_arg_t *sarg;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Star created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    star_output = (stream_t **) SNetMemAlloc( sizeof(stream_t *));
    star_output[0] = StreamCreate();

    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    sarg->input = input;
    sarg->output = star_output[0];
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_tags = type;
    sarg->guards = guards;
    sarg->is_incarnate = true;
    sarg->is_det = false;

    SNetEntitySpawn( StarBoxTask, (void*)sarg, ENTITY_star_nondet);
    dispatch_output = CollectorCreate( 1, star_output);

#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    dispatch_output = input;
  }
#endif /* DISTRIBUTED_SNET */
  return( dispatch_output);
}


/**
 * Star incarnate creation function
 */
stream_t *SNetStarIncarnate( stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  stream_t *output;
  star_arg_t *sarg;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Star incarnate created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    output = StreamCreate();

    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    sarg->input = input;
    sarg->output = output;
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_tags = type;
    sarg->guards = guards;
    sarg->is_incarnate = true;
    sarg->is_det = false;

    SNetEntitySpawn( StarBoxTask, (void*)sarg, ENTITY_star_nondet);
    
#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    output = input;
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}




/* ------------------------------------------------------------------------- */
/*  SNetStarDet                                                              */
/* ------------------------------------------------------------------------- */


/**
 * Det Star creation function
 */
stream_t *SNetStarDet(stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  stream_t **star_output, *dispatch_output;
  star_arg_t *sarg;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetStar created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    star_output = (stream_t **) SNetMemAlloc( sizeof(stream_t *));
    star_output[0] = StreamCreate();

    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    sarg->input = input;
    sarg->output = star_output[0];
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_tags = type;
    sarg->guards = guards;
    sarg->is_incarnate = false;
    sarg->is_det = true;

    SNetEntitySpawn( StarBoxTask, (void*)sarg, ENTITY_star_det);
    dispatch_output = CollectorCreate( 1, star_output);

#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    dispatch_output = input;
  }
#endif /* DISTRIBUTED_SNET */
  return(dispatch_output);
}




/**
 * Det star incarnate creation function
 */
stream_t *SNetStarDetIncarnate(stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  stream_t *output;
  star_arg_t *sarg;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetStar incarnate created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    output = StreamCreate();

    sarg = (star_arg_t *) SNetMemAlloc( sizeof(star_arg_t));
    sarg->input = input;
    sarg->output = output;
    sarg->box = box_a;
    sarg->selffun = box_b;
    sarg->exit_tags = type;
    sarg->guards = guards;
    sarg->is_incarnate = true;
    sarg->is_det = true;

    SNetEntitySpawn( StarBoxTask, (void*)sarg, ENTITY_star_det);
#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    output = input;
}
#endif /* DISTRIBUTED_SNET */
  return( output);
}


