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
static void *StarEntryThread(void* input)
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
  snet_record_t *temp_record;

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
      SNetUtilDebugNotice("moo");
      record_from_data_in = false;
      do {
        current_record = (snet_record_t*)SNetFbckBufTryGet(feedback_out,
                                                          &fmessage);
      } while(fmessage == FBCKBUF_blocked);
      if(current_record == NULL) {
        record_from_data_in = true;
        do {
          current_record = (snet_record_t*) SNetBufTryGet(data_in, &bmessage);
        } while(bmessage == BUF_blocked);
        SNetUtilDebugNotice("current_record: %x", (unsigned int) current_record);
        if(current_record == NULL) {
          SNetUtilDebugNotice("FeedbackStar: This should not happen, the "
                  "semaphore said >data ready!<, but I got NULL from both "
                  "streams.");
          continue;
        }
      }
    } else {
      SNetUtilDebugNotice("quack");
      record_from_data_in = true;
      do {
        current_record = (snet_record_t*)SNetBufTryGet(data_in, &bmessage);
      } while(bmessage == BUF_blocked);
      if(current_record == NULL) {
        record_from_data_in = false;
        do {
          current_record = (snet_record_t*)SNetFbckBufTryGet(feedback_out,
                                                              &fmessage);
        } while(fmessage == FBCKBUF_blocked);
        record_from_data_in = false;
        if(current_record == NULL) {
          SNetUtilDebugNotice("FeedbackStar: This should not happen, the "
                  "semaphore said >data ready!<, but I got NULL from both "
                  "streams.");
          continue;
        }
      }
    }
    SNetUtilDebugNotice("data in star entry, from %d", record_from_data_in);
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
          SNetUtilDebugNotice("terminator arrived! starting probes!");
          pre_termination = true;
          probing = true;
          probe_failed = false;
          SNetBufPut(body_in, SNetRecCreate(REC_probe));
          SNetRecDestroy(current_record);
        } else {
          SNetUtilDebugNotice("Star Entry: Unexpected termination token on "
                              "Feedback out! Destroying it");
          SNetRecDestroy(current_record);
        }
      break;

      case REC_probe:
        SNetUtilDebugNotice("star entry probed");
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
              probing = false;
              SNetBufPut(reject, current_record);
            }
          }
        }
      break;
    }
  }
  return NULL;
}

struct star_exit_input {
  snet_buffer_t *finish;
  snet_fbckbuffer_t *feedback_in;
  snet_buffer_t *body_out;
  snet_typeencoding_t *exittype;
  snet_expr_list_t *exitguards;
};

static void *StarExitThread(void *packed_input)
{
  struct star_exit_input *input = (struct star_exit_input*) packed_input;
  snet_buffer_t *finish = input->finish;
  snet_fbckbuffer_t *feedback_in = input->feedback_in;
  snet_buffer_t *body_out = input->body_out;
  snet_typeencoding_t *exittype = input->exittype;
  snet_expr_list_t *exitguards = input->exitguards;

  bool terminate = false;
  snet_record_t *current_record;
  snet_record_t *temp_record;

  snet_util_tree_t *sort_records = SNetUtilTreeCreate();
  snet_util_stack_t *curr_iter_stack;

  SNetMemFree(input);
  while(!terminate) {
    SNetUtilDebugNotice("star exit is alive");
    current_record = SNetBufGet(body_out);
    SNetUtilDebugNotice("STAR EXIT:: data in star exit");
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
          SNetUtilDebugNotice("foobar");
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
        SNetUtilDebugNotice("terminated");
        terminate = true;
        SNetFbckBufDestroy(feedback_in);
        SNetBufPut(finish, current_record);
        SNetBufBlockUntilEmpty(finish);
        SNetBufDestroy(finish);
      break;

      case REC_probe:
        SNetUtilDebugNotice("exit probed!");
        SNetFbckBufPut(feedback_in, current_record);
      break;
    }
  }
  SNetUtilDebugNotice("Star exit is dead now");
  return NULL;
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

  reject = SNetBufCreate(BUFFER_SIZE);
  finish = SNetBufCreate(BUFFER_SIZE);

  snet_buffer_t *data_out = CreateCollector(reject);
  SNetBufPut(reject, SNetRecCreate(REC_collect, finish));

  feedback = SNetFbckBufCreate();

  body_in = SNetBufCreate(BUFFER_SIZE);
  body_out = (*box_a)(body_in);

  struct star_entry_input *x;
  struct star_exit_input *y;

  x = SNetMemAlloc(sizeof(struct star_entry_input));
  x->data_in = data_in;
  x->feedback_out = feedback;
  x->body_in = body_in;
  x->reject = reject;
  x->exittype = type;
  x->exitguards = guards;
  SNetThreadCreate(&StarEntryThread, x, ENTITY_star_nondet);

  y = SNetMemAlloc(sizeof(struct star_exit_input));
  y->finish = finish;
  y->feedback_in = feedback;
  y->body_out = body_out;
  y->exittype = type;
  y->exitguards = guards;
  SNetThreadCreate(&StarExitThread, y, ENTITY_star_nondet);
  return data_out;
}

extern
snet_buffer_t *SNetStarIncarnate( snet_buffer_t *inbuf,
                            snet_typeencoding_t *type,
                               snet_expr_list_t *guards,
                                  snet_buffer_t *(*box_a)(snet_buffer_t*),
                                  snet_buffer_t *(*box_b)(snet_buffer_t*)) {
  SNetUtilDebugFatal("Hey, who called StarIncarnate?");
  return inbuf;
}

extern snet_buffer_t *SNetStarDet( snet_buffer_t *inbuf,
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
                                   snet_buffer_t *(*box_a)(snet_buffer_t*),
                                   snet_buffer_t *(*box_b)(snet_buffer_t*)) {
  SNetUtilDebugFatal("Detfeedbackstar not implemented.");
  return inbuf;
}
snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf,
                               snet_typeencoding_t *type,
                                  snet_expr_list_t *guards,
                                     snet_buffer_t *(*box_a)(snet_buffer_t*),
                                     snet_buffer_t *(*box_b)(snet_buffer_t*)) {

  SNetUtilDebugFatal("deterministic feedback star not done yet.");
  return NULL;
}
