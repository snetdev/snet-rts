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
#include "list.h"

#include "collectors.h"

#define FEEDBACK_STAR_DEBUG
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


struct incarnate_instance {
  int iteration_counter;
  snet_buffer_t *finished;
  bool has_next_instance;
};
enum data_status {
  NO_DATA, FROM_FEEDBACK, FROM_DATA_IN
};
struct input_state {
  snet_record_t *current_record;

  enum data_status data_status;

  int records_from_data_in;;
  int records_from_feedback;
};

/* @fn struct incarnate_instance *FindLatestInstance(
 *      instance_list_t *instance_list)
 * @brief returns the only incarnate_instance in the instance_list without
 *        a next incarnation.
 * @param instance_list the instance_list to search
 * @return the only instance without a next one
 */
static struct incarnate_instance*
FindLatestInstance(snet_util_list_t *instance_list)
{
  snet_util_list_iter_t *current_position;
  struct incarnate_instance *current_instance;
  struct incarnate_instance *result;

  current_position = SNetUtilListLast(instance_list);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    current_instance = SNetUtilListIterGet(current_position);
    current_position = SNetUtilListIterPrev(current_position);

    if(!current_instance->has_next_instance) {
      result = current_instance;
      break;
    }
  }
  return current_instance;
}

/* @fn struct incarnate_instance *CreateInstance(snet_util_list_t*instance_list,
 *        snet_record_t *reference_record)
 * @brief creates a new instance for the given record, inserts it into the list
 *        and registers the new stream with the collector
 * @param instance_list the list to insert the new instance into
 * @param reference_record the record to insert
 */
struct incarnate_instance*
CreateInstance(snet_util_list_t *instance_list,
               snet_record_t *reference_record) {
  struct incarnate_instance *latest_instance;
  snet_buffer_t *new_buffer;
  struct incarnate_instance *new_instance;

  latest_instance = FindLatestInstance(instance_list);

  new_buffer = SNetBufCreate(BUFFER_SIZE);

  new_instance = SNetMemAlloc(sizeof(struct incarnate_instance));
  new_instance->has_next_instance = false;
  new_instance->finished = new_buffer;
  new_instance->iteration_counter = SNetRecGetIteration(reference_record);
  instance_list = SNetUtilListAddEnd(instance_list, new_instance);

  latest_instance->has_next_instance = true;
  SNetBufPut(latest_instance->finished, SNetRecCreate(REC_collect, new_buffer));

  return new_instance;
}
/* @fn struct incarnate_instance *GetInstance(snet_util_list_t *instance_list,
 *              snet_record_t *reference_record)
 * @brief Searches the instance that corresponds to he current record
 * This searches for the instance that corresponds to the iteration
 * counter of the given record. If no such instance exists, a new instance
 * is created and returned.
 * @param instance_list the list to search and modify
 * @param current_record the record with the iteration counter
 * @return a corresponding instance
 */
static struct incarnate_instance *
GetCurrentInstance(snet_util_list_t *instance_list,
            snet_record_t *reference_record)
{
  snet_util_list_iter_t *current_position;
  struct incarnate_instance *current_instance;
  struct incarnate_instance *result;
  int target_iteration_counter;

  target_iteration_counter = SNetRecGetIteration(reference_record);
  result = NULL;
  current_position = SNetUtilListFirst(instance_list);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    current_instance = SNetUtilListIterGet(current_position);
    current_position = SNetUtilListIterNext(current_position);

    if(current_instance->iteration_counter == target_iteration_counter) {
      result = current_instance;
      break;
    }
  }
  if(result == NULL) {
    result = CreateInstance(instance_list, reference_record);
  }
  SNetUtilListIterDestroy(current_position);
  return result;
}


static bool
MatchesExitPattern( snet_record_t *rec,
                    snet_typeencoding_t *patterns,
                    snet_expr_list_t *guards)
{

  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  #ifdef FEEDBACK_STAR_DEBUG
    SNetUtilDebugNotice("Matches(%x, %x, %x) called",
        (unsigned int) rec, (unsigned int) patterns,
        (unsigned int) guards);
  #endif
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


/* @fn struct input_state *TryReadFromFreedback(struct input_state *input_state,
 *              snet_fbckbuffer_t *feedback)
 * @brief tries to read data from the given buffer, sets the data_status accordingly
 * This tries to read data from feedback. If data is found, data_status is
 * set to FROM_FEEDBACK, records_from_feedback is incremented and result is set
 * to the read record. Otherwise, data_status is set to NO_DATA and result is
 * set to NULL.
 * @param input_state the input_state-struct to modfiy
 * @param feedback the buffer to read from
 * @return a pointer to the given input_state-struct.
 */
static struct input_state*
TryReadFromFeedback(struct input_state *input_state,snet_fbckbuffer_t *feedback)
{
  snet_record_t *current_record;
  snet_fbckbuffer_msg_t fmessage;

  do {
    current_record = (snet_record_t*)SNetFbckBufTryGet(feedback,
                                                      &fmessage);
  } while(fmessage == FBCKBUF_blocked);

  if(current_record != NULL) {
    #ifdef FEEDBACK_STAR_DEBUG
      SNetUtilDebugNotice("FEEDBACK STAR %x: got input %x from feedback",
          (unsigned int) feedback, (unsigned int) current_record);
    #endif
    input_state->records_from_feedback++;
    input_state->current_record = current_record;
    input_state->data_status = FROM_FEEDBACK;
  } else {
    input_state->current_record = NULL;
    input_state->data_status = NO_DATA;
  }
  return input_state;
}

/* @fn struct input_state *TryReadFromDataIn(struct input_state *input_state,
 *              snet_buffer_t *data_in)
 * @brief tries to read data from the given buffer
 * This tries to read data from data_in. If data is found, data_status is
 * set to FROM_DATA_IN, records_from_data_in is incremented and result is set
 * to the read record. Otherwise, data_status is set to NO_DATA and result is
 * set to NULL.
 * @param input_state the input_state-struct to modfiy
 * @param feedback the buffer to read from
 * @return a pointer to the given input_state-struct.
 */
static struct input_state*
TryReadFromDataIn(struct input_state *input_state, snet_buffer_t *data_in)
{
  snet_record_t *current_record;
  snet_buffer_msg_t bmessage;

  do {
    current_record = (snet_record_t*) SNetBufTryGet(data_in, &bmessage);
  } while(bmessage == BUF_blocked);

  if(current_record != NULL) {
    #ifdef FEEDBACK_STAR_DEBUG
      SNetUtilDebugNotice("FEEDBACK STAR : got input %x from data in",
          (unsigned int) current_record);
    #endif
    input_state->records_from_data_in++;
    input_state->current_record = current_record;
    input_state->data_status = FROM_DATA_IN;
  } else {
    input_state->current_record = NULL;
    input_state->data_status = NO_DATA;
  }
  return input_state;
}

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
static bool TakeFromFeedback(int recs_from_feedback, int recs_from_data_in)
{
  /* arbitrary decision: every five records from feedback,
   * we take one from data_in
   */
  if(5*recs_from_data_in < recs_from_feedback) {
    return false;
  } else {
    return true;
  }
}

/* @fn struct input_state* GetInput(struct input_state *input_state,
 *        snet_buffer_t *data_in, snet_fbck_buffer_t *feedback_out)
 * @brief gets input either from feedback or data in
 * Depending on the input state, gets data from the feedback or
 * data_in and returns this. if both streams have no data,
 * input_state->data_status will be NO_DATA and an error will
 * be output.
 * @param input_state the state of the input
 * @param data_in the data in buffer
 * @param feedback the feedback buffer
 * @return a pointer to the input_state
 */
static struct input_state*
GetInput(struct input_state *input_state,
         snet_buffer_t *data_in,
         snet_fbckbuffer_t *feedback_out)
{
  int recs_from_data_in = input_state->records_from_data_in;
  int recs_from_feedback_out = input_state->records_from_feedback;

  #ifdef FEEDBACK_STAR_DEBUG
    SNetUtilDebugNotice("FEEDBACK STAR %x: getting input",
                          (unsigned int) feedback_out);
  #endif
  if(TakeFromFeedback(recs_from_feedback_out, recs_from_data_in)) {
    input_state = TryReadFromFeedback(input_state, feedback_out);
    if(input_state->data_status == NO_DATA) {
      input_state = TryReadFromDataIn(input_state, data_in);
    }
  } else {
    input_state = TryReadFromDataIn(input_state, data_in);
    if(input_state->data_status == NO_DATA) {
      input_state = TryReadFromFeedback(input_state, feedback_out);
    }
  }
  return input_state;
}

/* @fn void TerminateInstances(snet_util_list_t *instance_list)
 * @brief outputs a terminate token on all buffers of the instances
 *         and frees them.
 * @param instance_list the instances to be freed
 */
void TerminateInstances(snet_util_list_t *instance_list)
{
  snet_util_list_iter_t *current_position;
  snet_record_t *terminate_record;
  struct incarnate_instance *current_instance;

  current_position = SNetUtilListFirst(instance_list);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    current_instance = SNetUtilListIterGet(current_position);
    current_position = SNetUtilListIterNext(current_position);

    terminate_record = SNetRecCreate(REC_terminate);
    SNetBufPut(current_instance->finished, terminate_record);
    SNetMemFree(current_instance);
  }
  SNetUtilListIterDestroy(current_position);
}

struct star_entry_input {
  snet_buffer_t *data_in;
  snet_fbckbuffer_t *feedback_out;
  snet_buffer_t *body_in;
  snet_buffer_t *reject;
  snet_typeencoding_t *exittype;
  snet_expr_list_t *exitguards;
};

static void *StarEntryThread(void* input)
{
  struct star_entry_input *c_input = (struct star_entry_input*) input;
  snet_buffer_t *data_in = c_input->data_in;
  snet_fbckbuffer_t *feedback_out = c_input->feedback_out;
  snet_buffer_t *body_in = c_input->body_in;
  snet_buffer_t *reject = c_input->reject;
  snet_typeencoding_t *exittype = c_input->exittype;
  snet_expr_list_t *exitguards = c_input->exitguards;


  bool terminate = false;

  bool pre_termination;
  bool probing;
  bool probe_failed;

  struct input_state input_state;

  sem_t *sem;
  snet_record_t *temp_record;

  SNetMemFree(input);
  
  input_state.data_status = NO_DATA;
  input_state.records_from_data_in = 0;
  input_state.records_from_feedback = 0;
  input_state.current_record = NULL;

  sem = SNetMemAlloc(sizeof(sem_t));
  sem_init(sem, 0, 0);
  SNetBufRegisterDispatcher(data_in, sem);
  SNetFbckBufRegisterDispatcher(feedback_out, sem);

  while(!terminate) {
    sem_wait(sem);

    input_state = *GetInput(&input_state, data_in, feedback_out);
    if(input_state.data_status == NO_DATA) { continue; }

    switch(input_state.data_status) {
      case NO_DATA:
        SNetUtilDebugNotice("FEEDBACK_STAR %x: This should not happen, the "
                "semaphore said >data ready!<, but I got no data from both "
                "streams.", (unsigned int) feedback_out);
        continue;
      break; //unreachable

      case FROM_DATA_IN:
        /* data from the outer world. we need to behave like a StarBoxThread
         * + adding iteration counters to everything
         */
        switch(SNetRecGetDescriptor(input_state.current_record)) {
          case REC_data:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR %x: got data record %x",
                  (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif
            if(MatchesExitPattern(input_state.current_record,
                                  exittype, exitguards)) {
              #ifdef FEEDBACK_STAR_DEBUG
                SNetUtilDebugNotice("FEEDBACK_STAR %x: rejected",
                  (unsigned int) feedback_out);
              #endif
              SNetBufPut(reject, input_state.current_record);
            } else {
              #ifdef FEEDBACK_STAR_DEBUG
                SNetUtilDebugNotice("FEEDBACK_STAR %x: iterating it",
                    (unsigned int) feedback_out);
              #endif
              SNetRecAddIteration(input_state.current_record, 0);
              SNetBufPut(body_in, input_state.current_record);
            }
          break;

          case REC_sync:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR %x: sync record %x received",
                  (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif

            /* A synccell in front of us synced and fowards its input stream to
             * us. The sync-record is the last record output by the synccell,
             * thus, we can safely assume the buffer from the sync-cell to our
             * input and thus we can destroy it.
             */
            SNetBufDestroy(data_in);
            data_in = SNetRecGetBuffer(input_state.current_record);
            SNetRecDestroy(input_state.current_record);
          break;

          case REC_collect:
            SNetUtilDebugNotice("FEEDBACK %x: got unexpected collect record,"
                " discarding it", (unsigned int) feedback_out);
            SNetRecDestroy(input_state.current_record);
          break;

          case REC_sort_begin:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR %x: sort begin record %x"
                  " received", (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif
            SNetBufPut(reject, input_state.current_record);
            /* StarBoxThread has to check if a next instance exists
             * here. I do not have to check this if the data comes
             * from data_in, because at least 1 further instance
             * exists, thus, we have to pass this onward allways
             */
            temp_record = SNetRecCopy(input_state.current_record);
            SNetRecAddIteration(temp_record, 0);
            SNetBufPut(body_in, temp_record);
          break;

          case REC_sort_end:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR: %x: sort end record %x"
                  " received", (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif
            SNetBufPut(reject, input_state.current_record);

            /* see above */
            temp_record = SNetRecCopy(input_state.current_record);
            SNetRecAddIteration(temp_record, 0);
            SNetBufPut(body_in, temp_record);
          break;


          case REC_terminate:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR %x: termination record %x"
                  " received", (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif
            pre_termination = true;
            probing = true;
            probe_failed = false;
            SNetBufPut(body_in, SNetRecCreate(REC_probe));
            SNetRecDestroy(input_state.current_record);
          break;

          case REC_probe:
            #ifdef FEEDBACK_STAR_DEBUG
              SNetUtilDebugNotice("FEEDBACK_STAR %x: probe %x received",
                  (unsigned int) feedback_out,
                  (unsigned int) input_state.current_record);
            #endif
            probing = true;
            probe_failed = false;
            SNetBufPut(body_in, input_state.current_record);
          break;
        }
      break;

      case FROM_FEEDBACK:
        /* if we get here, we are part of a StarIncarnate.
         * Except for the probe-logic everything has been done in StarExit
         * already.
         */
        switch(SNetRecGetDescriptor(input_state.current_record)) {
          case REC_data:
          case REC_sort_begin:
          case REC_sort_end:
            probe_failed = true;
            SNetBufPut(body_in, input_state.current_record);
          break;

          case REC_sync:
            SNetUtilDebugFatal("FEEDBACK_STAR %x: received unexpected"
                " sync-record on feedback", (unsigned int) feedback_out);
          break;

          case REC_collect:
            SNetUtilDebugFatal("FEEDBACK_STAR %x: received unexpected"
                " collect record on feedback", (unsigned int) feedback_out);
          break;

          case REC_terminate:
            SNetUtilDebugFatal("FEEDBACK_STAR %x: received unexpected"
                " terminate record on feedback", (unsigned int) feedback_out);
          break;

          case REC_probe:
            if(probe_failed) {
              probing = true;
              probe_failed = false;
              SNetBufPut(body_in, input_state.current_record);
            } else {
              if(pre_termination) {
                terminate = true;
                SNetBufPut(reject, SNetRecCreate(REC_terminate));
                SNetBufPut(body_in, SNetRecCreate(REC_terminate));
                SNetBufBlockUntilEmpty(body_in);
                SNetBufDestroy(body_in);
              } else {
                probing = false;
                SNetBufPut(reject, input_state.current_record);
              }
            }
          break;
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

  snet_util_list_t *instance_list;
  struct incarnate_instance *current_instance;

  instance_list = SNetUtilListCreate();
  current_instance = SNetMemAlloc(sizeof(struct incarnate_instance));
  current_instance->finished = finish;
  current_instance->has_next_instance = false;
  current_instance->iteration_counter = 0;
  instance_list = SNetUtilListAddEnd(instance_list, current_instance);

  SNetMemFree(input);
  while(!terminate) {
    current_record = SNetBufGet(body_out);

    switch(SNetRecGetDescriptor(current_record)) {
      case REC_data:
        #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: data record %x",
              (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        if(!SNetRecHasIteration(current_record)) {
          SNetUtilDebugFatal("FEEDBACK_STAR %x: missing iteration counters",
              (unsigned int) feedback_in);
        }

        if(MatchesExitPattern(current_record, exittype, exitguards)) {
          SNetRecRemoveIteration(current_record);
          SNetBufPut(current_instance->finished, current_record);
        } else {
          SNetRecIncIteration(current_record);
          SNetFbckBufPut(feedback_in, current_record);
        }
      break;

      case REC_sync:
        #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: sync record %x",
              (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        body_out = SNetRecGetBuffer(current_record);
        SNetRecDestroy(current_record);
      break;

      case REC_collect:
        SNetUtilDebugNotice("Star Exit: Unexpected collector token, "
                            "destroying it");
        SNetRecDestroy(current_record);
      break;


     case REC_sort_begin:
        #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: sort_begin record %x",
              (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        if(!SNetRecHasIteration(current_record)) {
          SNetUtilDebugFatal("FEEDBACK_STAR %x: missing iteration counters",
              (unsigned int) feedback_in);
        }
        current_instance = GetCurrentInstance(instance_list, current_record);
        if(current_instance->has_next_instance) {
          temp_record = SNetRecCopy(current_record);
          SNetRecRemoveIteration(temp_record);
          SNetBufPut(current_instance->finished, temp_record);

          SNetRecIncIteration(current_record);
          SNetFbckBufPut(feedback_in, current_record);
        }
      break;

      case REC_sort_end:
        #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: sort_end record %x",
            (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        if(!SNetRecHasIteration(current_record)) {
          SNetUtilDebugFatal("FEEDBACK_STAR %x: missing iteration counters",
              (unsigned int) feedback_in);
        }
        current_instance = GetCurrentInstance(instance_list, current_record);
        if(current_instance->has_next_instance) {
          temp_record = SNetRecCopy(current_record);
          SNetRecRemoveIteration(temp_record);
          SNetBufPut(current_instance->finished, temp_record);

          SNetRecIncIteration(current_record);
          SNetFbckBufPut(feedback_in, current_record);
        }
      break;

      case REC_terminate:
        #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: termination record %x",
              (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        terminate = true;
      break;

      case REC_probe:
             #ifdef FEEDBACK_STAR_DEBUG
          SNetUtilDebugNotice("FEEDBACK_STAR %x: probe record %x",
              (unsigned int) feedback_in, (unsigned int) current_record);
        #endif
        SNetFbckBufPut(feedback_in, current_record);
      break;
    }
  }
  TerminateInstances(instance_list);
  SNetUtilListDestroy(instance_list);
  SNetFbckBufDestroy(feedback_in);
  SNetBufPut(finish, current_record);
  SNetBufBlockUntilEmpty(finish);
  SNetBufDestroy(finish);
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
