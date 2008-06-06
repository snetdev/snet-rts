#include <semaphore.h>

#include "star.h"

#include "memfun.h"
#include "buffer.h"
#include "feedbackbuffer.h"
#include "typeencode.h"
#include "expression.h"
#include "bool.h"
#include "threading.h"
#include "debug.h"
#include "tree.h"

#include "collectors.h"


/* (I hope you use a monospace font!)
 * Terms used: Data in is the inbuffer, Data out is the outbuffer
 * Shown is the nondeterministic feedback loop.
 *                                     +----------+
 *                   /--(feedback out)-| feedback |<-(feedback in)-\
 *                   |                 +----------+                |
 *                   V                                             |
 *                /-----\                                          |
 *               /       \              +------+               /------\
 *               | Star  |              |      |               | Star |
 * --(Data in)-->|       |--(body in)-->| Body |--(body out)-->|      |
 *               | Entry |              |      |               | Exit |
 *               \       /              +------+               \------/
 *                \-----/                                         |
 *                   |                                            |
 *                   |                                            |
 *                   \--(reject)---\       /------(finish)--------/
 *                                 |       |
 *                                 |       |
 *                                 V       V
 *                               +-----------+
 *                               | Collector |
 *                               +-----------+
 *                                     |
 *                                     \----(Data out)---->
 */

static bool
MatchesExitPattern( snet_record_t *rec,
                    snet_typeencoding_t *patterns,
                    snet_expr_list_t *guards)
{

  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    is_match = true;
    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( patterns, i+1);
      is_match = SNetTencPatternMatches(pat, rec);
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

struct star_entry_input {
  snet_buffer_t *data_in;
  snet_fbckbuffer_t *feedback_out;
  snet_buffer_t *body_in;
  snet_buffer_t *reject;
  snet_typeencoding_t *exittype;
  snet_expr_list_t *exitguards;
};
/**<!--**********************************************************************-->
 *
 * @fn bool TakeFromFeedback(int recs_from_feedback, int recs_from_data_in)
 *
 * @brief returns true if star entry is supposed to take data from the feedback
 *
 *   This function can be made more clever. We need to 'interleave' data from
 *   the outer world with data from the feedback, as one might stuff enough
 *   nonterminating records into the star so it is saturated and won't read
 *   data from the outer world anymore.
 *   If sensible data follows after hat nonsense, this star implementation
 *   would be wrong if we didnt interleave both data inputs.
 *
 * @return true if we are supposed to take data from feedback or from data_in
 *
 ******************************************************************************/
static bool TakeFromFeedback(int recs_from_feedback, int recs_from_data_in) {
  /* every five records from feedback, we take one from data_in */
  if(5*recs_from_data_in < recs_from_feedback) {
    return false;
  } else {
    return true;
  }
}
static void
StarEntryThread(void* input)
{
  struct star_entry_input *c_input = (struct star_entry_input*) input;
  snet_buffer_t *data_in = c_input->data_in;
  snet_fbckbuffer_t *feedback_out = c_input->feedback_out;
  snet_buffer_t *body_in = c_input->body_in;
  snet_buffer_t *reject = c_input->reject;
  snet_typeencoding_t *exittype = c_input->exittype;
  snet_expr_list_t *exitguards = c_input->exitguards;
  SNetMemFree(input);

  int recs_from_feedback = 0;
  int recs_from_data_in = 0;
  bool terminate = false;
  bool record_from_data_in;

  bool pre_termination;
  bool probing;
  bool probe_failed;

  sem_t *sem;
  snet_record_t *current_record;

  sem = SNetMemAlloc(sizeof(sem_t));
  sem_init(sem, 0, 0);
  SNetBufRegisterDispatcher(data_in, sem);
  SNetFbckBufRegisterDispatcher(feedback_out, sem);

  snet_fbckbuffer_msg_t fmessage;
  snet_buffer_msg_t bmessage;
  while(!terminate) {
    sem_wait(sem);

    /* depending on the selection function, read data or restart the loop. */
    if(TakeFromFeedback(recs_from_feedback, recs_from_data_in)) {
      current_record = (snet_record_t*)SNetFbckBufTryGet(feedback_out,
                                                          &fmessage);
      record_from_data_in = false;
      if(current_record == NULL) {
        current_record = (snet_record_t*) SNetBufTryGet(data_in, &bmessage);
        if(current_record == NULL) {
          SNetUtilDebugNotice("FeedbackStar: This should not happen, the "
                  "semaphore said >data ready!<, but I got NULL from both "
                  "streams.");
          continue;
        }
        record_from_data_in = true;
      }
    } else {
      current_record = (snet_record_t*)SNetBufTryGet(data_in, &bmessage);
      record_from_data_in = true;
      if(current_record == NULL) {
        current_record = (snet_record_t*)SNetFbckBufTryGet(feedback_out,
                                                              &fmessage);
        record_from_data_in = false;
        if(current_record == NULL) {
          SNetUtilDebugNotice("FeedbackStar: This should not happen, the "
                  "semaphore said >data ready!<, but I got NULL from both "
                  "streams.");
          continue;
        }
      }
    }
    switch(SNetRecGetDescriptor(current_record)) {
      case REC_data:
        if(record_from_data_in) {
          if(MatchesExitPattern(current_record, exittype, exitguards)) {
            SNetBufPut(reject, current_record);
          } else {
            SNetRecAddIteration(current_record, 0);
            SNetBufPut(body_in, current_record);
          }
        } else {
          probe_failed = true;
          SNetRecIncIteration(current_record);
          SNetBufPut(body_in, current_record);
        }
      break;

      case REC_sync:
        /* a synccell in front of us synced */
        data_in = SNetRecGetBuffer(current_record);
        SNetRecDestroy(current_record);
      break;

      case REC_collect:
        SNetUtilDebugNotice("Feedback Star:Unhandled Collect Record,"
                            " killing it");
        SNetRecDestroy(current_record);
      break;

      case REC_sort_begin:
        if(!record_from_data_in) {
          probe_failed = true;
        }
        SNetBufPut(body_in, SNetRecCreate(REC_sort_begin,
                                SNetRecGetLevel(current_record),
                                SNetRecGetNum(current_record)));
        SNetBufPut(reject, current_record);
      break;

      case REC_sort_end:
        if(!record_from_data_in) {
          probe_failed = true;
        }
        SNetBufPut(body_in, SNetRecCreate(REC_sort_end,
                                SNetRecGetLevel(current_record),
                                SNetRecGetNum(current_record)));
        SNetBufPut(reject, current_record);
      break;

      case REC_terminate:
        if(record_from_data_in) {
          pre_termination = true;
          probing = true;
          probe_failed = false;
          SNetBufPut(body_in, current_record);
          SNetRecDestroy(current_record);
        } else {
          SNetUtilDebugNotice("Star Entry: Unexpected termination token on "
                              "Feedback out! Destroying it");
          SNetRecDestroy(current_record);
        }
      break;

      case REC_probe:
        if(record_from_data_in) {
          probing = true;
          probe_failed = false;
          SNetBufPut(body_in, current_record);
        } else {
          if(probe_failed) {
            probing = true;
            probe_failed = false;
            SNetBufPut(body_in, current_record);
          } else {
            if(pre_termination) {
              terminate = true;
              SNetBufPut(body_in, SNetRecCreate(REC_terminate));
              SNetBufBlockUntilEmpty(body_in);
              SNetBufDestroy(body_in);
            } else {
              SNetBufPut(reject, current_record);
            }
          }
        }
      break;
    }
  }
}

static void StarExitThread(snet_buffer_t *finish,
                           snet_fbckbuffer_t *feedback_in,
                           snet_buffer_t *body_out,
                           snet_typeencoding_t *exittype,
                           snet_expr_list_t *exitguards) 
{
  bool terminate = false;
  snet_record_t *current_record;
  snet_record_t *temp_record;

  snet_util_tree_t *sort_records = SNetUtilTreeCreate();
  snet_util_stack_t *curr_iter_stack;

  while(!terminate) {
    current_record = SNetBufGet(body_out);
    switch(SNetRecGetDescriptor(current_record)) {
      case REC_data:
        if(MatchesExitPattern(current_record, exittype, exitguards)) {
          SNetRecRemoveIteration(current_record);
          SNetBufPut(finish, current_record);
        } else {
          curr_iter_stack = SNetRecGetIterationStack(current_record);
          if(SNetUtilTreeContains(sort_records, curr_iter_stack)) {
            SNetFbckBufPut(feedback_in,
                      SNetUtilTreeGet(sort_records, curr_iter_stack));
            SNetUtilTreeDelete(sort_records, curr_iter_stack);
          }
          SNetFbckBufPut(feedback_in, current_record);
        }
      break;

      case REC_sync:
        body_out = SNetRecGetBuffer(current_record);
        SNetRecDestroy(current_record);
      break;

      case REC_collect:
        SNetUtilDebugNotice("Star Exit: Unexpected collector token, "
                            "destroying it");
        SNetRecDestroy(current_record);
      break;

      case REC_sort_begin:
        SNetUtilTreeSet(sort_records, SNetRecGetIterationStack(current_record),
                        current_record);
        SNetBufPut(finish, SNetRecCopy(current_record));
      break;

      case REC_sort_end:
        /* if the corresponding sort start record is still in
         * the sort records tree, then there was no data record
         * between sort_start and the corresponding sort_end.
         * Thus, we can discard both sort records.
         */
        curr_iter_stack = SNetRecGetIterationStack(current_record);
        if(SNetUtilTreeContains(sort_records, curr_iter_stack)) {
          /* I assume: no sort records disappeared in dark alleys
           * Thus, this sort_begin_record in the tree belongs to
           * this sort_end.
           * Thus, I discard both.
           */
          SNetRecDestroy(current_record);
          temp_record = SNetUtilTreeGet(sort_records, curr_iter_stack);
          SNetRecDestroy(temp_record);
        } else {
          /* the sort record was output somewhere already. */
          SNetFbckBufPut(feedback_in, current_record);
          SNetBufPut(finish, current_record);
        }
      break;

      case REC_terminate:
        terminate = true;
        SNetFbckBufDestroy(feedback_in);
        SNetBufPut(finish, current_record);
        SNetBufBlockUntilEmpty(finish);
        SNetBufDestroy(finish);
      break;

      case REC_probe:
        SNetFbckBufPut(feedback_in, current_record);
      break;
    }
  }
}
/**<!--**********************************************************************-->
 *
 * @fn snet_buffer_t *SNetStar(snet_buffer_t *inbuf,
 *                                snet_typeencoding_t *type,
 *                                snet_expr_list_t *guards,
 *                                snet_buffer_t* (*box_a)(snet_buffer_t*),
 *                                snet_buffer_t* (*box_b)(snet_buffer_t*))
 *
 * @brief creates a star that reads from inbuf and outputs things to outbuf
 *
 *        type :: [0..i] => Variants, the output variants of the star
 *        guards :: [0..i] => Guards. Some record can exit the star iff
 *                guards[x] evaluates to true and the record is a subtype
 *                of type[x]
 *
 * @param inbuf the input buffer of the star
 * @param type the output types
 * @param guards if the guards for the output types
 * @param box_a the body of the network
 * @param box_b the function of the star itself?
 *
 * @return the output buffer of the feedback star
 */
snet_buffer_t *SNetStar( snet_buffer_t *data_in,
                                snet_typeencoding_t *type,
                                snet_expr_list_t *guards,
                                snet_buffer_t* (*box_a)(snet_buffer_t*),
                                snet_buffer_t* (*box_b)(snet_buffer_t*))
{
  snet_buffer_t *body_in;
  snet_buffer_t *body_out;

  snet_fbckbuffer_t *feedback;

  snet_buffer_t *reject;
  snet_buffer_t *finish;

  snet_buffer_t *data_out = CreateCollector(reject);
  SNetBufPut(reject, SNetRecCreate(REC_collect, finish));

  feedback = SNetFbckBufCreate(BUFFER_SIZE);

  body_in = SNetBufCreate(BUFFER_SIZE);
  body_out = (*box_a)(body_in);

  struct star_entry_input *x = SNetMemAlloc(sizeof(struct star_entry_input));
  x->data_in = data_in;
  x->feedback_out = feedback;
  x->body_in = body_in;
  x->reject = reject;
  x->exittype = type;
  x->exitguards = guards;
  return NULL;
}

snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf,
                               snet_typeencoding_t *type,
                                  snet_expr_list_t *guards,
                                     snet_buffer_t *(*box_a)(snet_buffer_t*),
                                     snet_buffer_t *(*box_b)(snet_buffer_t*)) {

  SNetUtilDebugFatal("deterministic feedback star not done yet.");
  return NULL;
}
