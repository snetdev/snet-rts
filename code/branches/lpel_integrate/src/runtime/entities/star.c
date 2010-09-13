#include "star.h"

#include "record.h"
#include "typeencode.h"
#include "threading.h"
#include "buffer.h"
#include "expression.h"
#include "handle.h"
#include "memfun.h"
#include "serial.h"
#include "debug.h"
#include "collectors.h"

#include "stream.h"
#include "task.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#include "fun.h"
#endif /* DISTRIBUTED_SNET */

//#define STAR_DEBUG

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



/**
 * Star component task
 */
static void StarBoxTask( task_t *self, void *arg)
{
#ifdef STAR_DEBUG
  char record_message[100];
#endif
  snet_handle_t *hnd = (snet_handle_t*)arg;
  snet_startup_fun_t box;
  snet_startup_fun_t selffun;
  stream_mh_t *instream;
  stream_mh_t *outstream; /* the stream to the collector */
  /* The stream to the next instance;
     a non-null value indicates that the instance has been created. */
  stream_mh_t *nextstream=NULL;
  bool terminate = false;
  snet_typeencoding_t *exit_tags;
  snet_record_t *rec;
  snet_expr_list_t *guards;
  bool is_det, is_incarnate;
  /* for deterministic variant: */
  int counter = 0;

#ifdef DISTRIBUTED_SNET
  int node_id;
  snet_fun_id_t fun_id;
#endif /* DISTRIBUTED_SNET */

  instream  = StreamOpen( self, SNetHndGetInput(hnd),   'r');
  outstream = StreamOpen( self, SNetHndGetOutput( hnd), 'w');

  exit_tags = SNetHndGetType( hnd);
  box = SNetHndGetBoxfunA( hnd);
  selffun = SNetHndGetBoxfunB( hnd);
  guards = SNetHndGetGuardList( hnd);
  is_det = SNetHndIsDet( hnd);
  is_incarnate = SNetHndIsIncarnate( hnd); 


#ifdef DISTRIBUTED_SNET
  node_id = SNetIDServiceGetNodeID();
  if(!SNetDistFunFun2ID(box, &fun_id)) {
    /* TODO: This is an error!*/
  }
#endif /* DISTRIBUTED_SNET */

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = StreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        if( MatchesExitPattern( rec, exit_tags, guards)) {
#ifdef STAR_DEBUG
          SNetUtilDebugDumpRecord(rec, record_message);
          SNetUtilDebugNotice("STAR %p stopping iterations of %s",
                              outstream,
                              record_message);
#endif
          /* send rec to collector */
          StreamWrite( outstream, rec);
          if (is_det && !is_incarnate) {
            /* append new sort record */
            StreamWrite( outstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
          }
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
#ifdef STAR_DEBUG
            SNetUtilDebugNotice("STAR %p creating new instance",
                                outstream);
#endif
            // register new buffer with dispatcher,
            // starstream is returned by selffun, which is SNetStarIncarnate
#ifdef DISTRIBUTED_SNET
            {
              snet_info_t *info;
              info = SNetInfoInit();
              SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, node_id));
              starstream = SNetSerial(nextstream_addr, info, node_id , box, selffun);
              starstream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), starstream);
              SNetInfoDestroy(info);
            }
#else
            /* starstream is the outstream of BOX + STAR_INC serial */
            starstream = SNetSerial(nextstream_addr, box, selffun);
#ifdef STAR_DEBUG
            SNetUtilDebugNotice("STAR %p has created a new instance",
                                outstream);
#endif
#endif /* DISTRIBUTED_SNET */
            /* notify collector about the new instance */
            StreamWrite( outstream, SNetRecCreate(REC_collect, starstream));
          }
#ifdef STAR_DEBUG
          SNetUtilDebugDumpRecord(rec, record_message);
          SNetUtilDebugNotice("STAR %p: outputting %s to %p",
                              outstream,
                              record_message,
                              nextstream);
#endif
          /* send the record to the instance */
          StreamWrite( nextstream, rec);
          /* deterministic non-incarnate has to append control records */
          if (is_det && !is_incarnate) {
            /* append new sort record */
            StreamWrite( nextstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
            /* also send new sort record to collector */
            StreamWrite( outstream,
                SNetRecCreate( REC_sort_end, 0, counter) );
          }
        }
        break;

      case REC_sync:
        {
          stream_t *newstream = SNetRecGetStream( rec);
#ifdef STAR_DEBUG
          SNetUtilDebugNotice("STAR %p: resetting input stream to %p",
              outstream,
              newstream);
#endif
          StreamReplace( instream, newstream);
          SNetHndSetInput(hnd, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        SNetUtilDebugNotice("[STAR] unhandled control record,"
                            " destroying it\n\n");
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
                  (!is_incarnate)? rec_lvl+1 : rec_lvl,
                  SNetRecGetNum(rec) )
                );
          }

          /* send the original one to the collector */
          if (!is_incarnate) {
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
        SNetUtilDebugNotice("[Star] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  StreamClose(instream, true);

  if( nextstream != NULL) {
    StreamClose( nextstream, false);
  }
  StreamClose( outstream, false);

  /* destroy the handle*/
  SNetHndDestroy( hnd);
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
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Star created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    star_output = (stream_t **) SNetMemAlloc( sizeof(stream_t *));
    star_output[0] = StreamCreate();
    hnd = SNetHndCreate(HND_star,
        input, star_output[0], box_a, box_b, type, guards, false, false);
    SNetEntitySpawn( StarBoxTask, hnd, ENTITY_star_nondet);
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
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Star incarnate created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    output = StreamCreate();
    hnd = SNetHndCreate(HND_star,
        input, output, box_a, box_b, type, guards, true, false);
    SNetEntitySpawn( StarBoxTask, hnd, ENTITY_star_nondet);
    
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
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetStar created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    star_output = (stream_t **) SNetMemAlloc( sizeof(stream_t *));
    star_output[0] = StreamCreate();
    hnd = SNetHndCreate( HND_star, 
        input, star_output[0], box_a, box_b, type, guards, false, true);

#ifdef STAR_DEBUG
    SNetUtilDebugNotice("-");
    SNetUtilDebugNotice("| DET STAR CREATED");
    SNetUtilDebugNotice("| input: %p", input);
    SNetUtilDebugNotice("| output: %p", star_output);
    SNetUtilDebugNotice("-");
#endif
    SNetEntitySpawn( StarBoxTask, hnd, ENTITY_star_det);
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
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetStar incarnate created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    output = StreamCreate();
    hnd = SNetHndCreate( HND_star,
        input, output, box_a, box_b, type, guards, true, true);
#ifdef STAR_DEBUG
    SNetUtilDebugNotice("-");
    SNetUtilDebugNotice("| DET STAR INCARNATE CREATED");
    SNetUtilDebugNotice("| input: %p", input);
    SNetUtilDebugNotice("| output: %p", output);
    SNetUtilDebugNotice("-");
#endif
    SNetEntitySpawn( StarBoxTask, hnd, ENTITY_star_det);
#ifdef DISTRIBUTED_SNET
  } else {
    SNetEdestroyList( guards);
    SNetDestroyTypeEncoding( type);
    output = input;
}
#endif /* DISTRIBUTED_SNET */
  return( output);
}


