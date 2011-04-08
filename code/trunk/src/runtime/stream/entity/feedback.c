#include <assert.h>

#include "snetentities.h"

#include "typeencode.h"
#include "expression.h"
#include "memfun.h"

#include "threading.h"
#include "distribution.h"



static bool MatchesBackPattern( snet_record_t *rec,
    snet_typeencoding_t *back_patterns, snet_expr_list_t *guards)
{
  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( back_patterns); i++) {
    is_match = true;

    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( back_patterns, i+1);
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





/******************************************************************************
 * Feedback collector
 *****************************************************************************/

typedef struct {
  snet_stream_t *in, *fbi, *out;
} fbcoll_arg_t;

enum fbcoll_mode {
  FBCOLL_IN,
  FBCOLL_FB1,
  FBCOLL_FB0
};


struct fbcoll_state {
    snet_stream_desc_t *instream;
    snet_stream_desc_t *outstream;
    snet_stream_desc_t *backstream;
    bool terminate;
    enum fbcoll_mode mode;
};

/* helper functions to handle mode the feedback collector is in */


static void FbCollReadIn(struct fbcoll_state *state)
{
  snet_record_t *rec;

  assert( false == state->terminate );

  /* read from input stream */
  rec = SNetStreamRead( state->instream);

  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* relay data record */
      SNetStreamWrite( state->outstream, rec);
      /* append a sort record */
      SNetStreamWrite(
          state->outstream,
          SNetRecCreate( REC_sort_end, 0, 1 ) /*type, lvl, num*/
          );
      /* mode switch to FB1 */
      state->mode = FBCOLL_FB1;
      break;

    case REC_sort_end:
      /* increase the level and forward */
      SNetRecSetLevel( rec, SNetRecGetLevel(rec)+1);
      SNetStreamWrite( state->outstream, rec);
      break;

    case REC_terminate:
      state->terminate = true;
      SNetStreamWrite( state->outstream, rec);
      /* note that no sort record has to be appended */
      break;

    case REC_sync:
      SNetStreamReplace( state->instream, SNetRecGetStream( rec));
      SNetRecDestroy( rec);
      break;

    case REC_collect:
    default:
      assert(0);
      /* if ignoring, at least destroy ... */
      SNetRecDestroy( rec);
      break;
  }
}


static void FbCollReadFbi(struct fbcoll_state *state)
{
  snet_record_t *rec;

  assert( false == state->terminate );

  /* read from feedback stream */
  rec = SNetStreamRead( state->backstream);

  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* relay data record */
      SNetStreamWrite( state->outstream, rec);
      /* mode switch to FB0 (there is a next iteration) */
      state->mode = FBCOLL_FB0;
      break;

    case REC_sort_end:
      assert( 0 == SNetRecGetLevel(rec) );
      switch(state->mode) {
        case FBCOLL_FB0:
          state->mode = FBCOLL_FB1;
          /* increase counter (non-functional) */
          SNetRecSetNum( rec, SNetRecGetNum(rec)+1);
          SNetStreamWrite( state->outstream, rec);
          break;
        case FBCOLL_FB1:
          state->mode = FBCOLL_IN;
          /* kill the sort record */
          SNetRecDestroy( rec);
          break;
        default: assert(0);
      }
      break;

    case REC_terminate:
    case REC_sync:
    case REC_collect:
    default:
      assert(0);
      /* if ignoring, at least destroy ... */
      SNetRecDestroy( rec);
      break;
  }

}


/**
 * The feedback collector, the entry point of the
 * feedback combinator loop
 */
static void FeedbackCollTask( snet_entity_t *self, void *arg)
{
  fbcoll_arg_t *fbcarg = (fbcoll_arg_t *)arg;
  struct fbcoll_state state;

  /* initialise state */
  state.terminate = false;
  state.mode = FBCOLL_IN;

  state.instream   = SNetStreamOpen( self, fbcarg->in,  'r');
  state.backstream = SNetStreamOpen( self, fbcarg->fbi, 'r');
  state.outstream  = SNetStreamOpen( self, fbcarg->out, 'w');
  SNetMemFree( fbcarg);

  /* MAIN LOOP */
  while( !state.terminate) {

    /* which stream to read from is mode dependent */
    switch(state.mode) {
      case FBCOLL_IN:
        FbCollReadIn(&state);
        break;
      case FBCOLL_FB1:
      case FBCOLL_FB0:
        FbCollReadFbi(&state);
        break;
      default: assert(0); /* should not be reached */
    }

  } /* END OF MAIN LOOP */

  SNetStreamClose(state.instream,   true);
  SNetStreamClose(state.backstream, true);
  SNetStreamClose(state.outstream,  false);

}




/******************************************************************************
 * Feedback dispatcher
 *****************************************************************************/


typedef struct {
  snet_stream_t *in, *out, *fbo;
  snet_typeencoding_t *back_patterns;
  snet_expr_list_t *guards;
} fbdisp_arg_t;


/**
 * The feedback dispatcher, at the end of the
 * feedback combinator loop
 */
static void FeedbackDispTask( snet_entity_t *self, void *arg)
{
  fbdisp_arg_t *fbdarg = (fbdisp_arg_t *)arg;

  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *backstream;
  bool terminate = false;
  snet_record_t *rec;

  instream   = SNetStreamOpen( self, fbdarg->in,  'r');
  outstream  = SNetStreamOpen( self, fbdarg->out, 'w');
  backstream = SNetStreamOpen( self, fbdarg->fbo, 'w');

  /* MAIN LOOP */
  while( !terminate) {

    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* route data record */
        if( MatchesBackPattern( rec, fbdarg->back_patterns, fbdarg->guards)) {
          /* send rec back into the loop */
          SNetStreamWrite( backstream, rec);
        } else {
          /* send to outpute */
          SNetStreamWrite( outstream, rec);
        }
        break;

      case REC_sort_end:
        {
          int lvl = SNetRecGetLevel(rec);
          if ( 0 == lvl ) {
            SNetStreamWrite( backstream, rec);
          } else {
            assert( lvl > 0 );
            SNetRecSetLevel( rec, lvl-1);
            SNetStreamWrite( outstream, rec);
          }
        }
        break;

      case REC_terminate:
        terminate = true;
        SNetStreamWrite( outstream, rec);
        /* note that no terminate record is sent in the backloop */
        break;

      case REC_sync:
        SNetStreamReplace( instream, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
      default:
        assert(0);
        /* if ignoring, at least destroy ... */
        SNetRecDestroy( rec);
        break;
    }

  } /* END OF MAIN LOOP */

  SNetStreamClose(instream,   true);
  SNetStreamClose(outstream,  false);
  SNetStreamClose(backstream, false);

  SNetDestroyTypeEncoding( fbdarg->back_patterns);
  SNetEdestroyList( fbdarg->guards);
  SNetMemFree( fbdarg);
}




/****************************************************************************/
/* CREATION FUNCTION                                                        */
/****************************************************************************/
snet_stream_t *SNetFeedback( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_typeencoding_t *back_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a
    )
{
  snet_stream_t *output;

  input = SNetRouteUpdate(info, input, location);
  if(location == SNetNodeLocation) {
    snet_stream_t *into_op, *from_op, *back;
    fbcoll_arg_t *fbcarg;
    fbdisp_arg_t *fbdarg;

    /* create streams */
    into_op = SNetStreamCreate(0);
    back    = SNetStreamCreate(0);
    output  = SNetStreamCreate(0);

    /* create the feedback collector */
    fbcarg = SNetMemAlloc( sizeof( fbcoll_arg_t));
    fbcarg->in = input;
    fbcarg->fbi = back;
    fbcarg->out = into_op;
    SNetEntitySpawn( ENTITY_FBCOLL, FeedbackCollTask, (void*)fbcarg );

    /* create the instance network */
    from_op = box_a(into_op, info, location);

    /* create the feedback dispatcher */
    fbdarg = SNetMemAlloc( sizeof( fbdisp_arg_t));
    fbdarg->in = from_op;
    fbdarg->fbo = back;
    fbdarg->out = output;
    fbdarg->back_patterns = back_patterns;
    fbdarg->guards = guards;
    SNetEntitySpawn( ENTITY_FBDISP, FeedbackDispTask, (void*)fbdarg );

  } else {
    output = input;
  }

  return( output);


}

