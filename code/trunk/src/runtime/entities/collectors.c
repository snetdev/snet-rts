#include <stdlib.h>

#include "collectors.h"
#include "memfun.h"
#include "handle.h"
#include "record.h"
#include "bool.h"
#include "list.h"
#include "debug.h"
#include "stream_layer.h"

//#define COLLECTOR_DEBUG
typedef struct {
  snet_tl_stream_t *from_stream;
  snet_tl_stream_t *to_stream;
  char *desc;

  bool is_det;
} dispatch_info_t;

typedef struct {
  snet_tl_stream_t *stream;
  snet_record_t *current;
} snet_collect_elem_t;


/* @fn void RemoveProbes(snet_util_list_t *streams, snet_tl_stream_t *output)
 * @brief removes the probes on all streams in the list and passes one
 *        to the outer world
 * @param buffers the buffers to remove a probe on
 * @param outbuf the outbuf to send a probe to
 */
static void RemoveProbes(snet_util_list_t *streams, snet_tl_stream_t *output)
{
  snet_util_list_iter_t *check_position;
  snet_collect_elem_t *tmp_elem;
  snet_record_t *temp_record;

  check_position = SNetUtilListFirst(streams);
  while(SNetUtilListIterHasNext(check_position)) {
    tmp_elem = SNetUtilListIterGet(check_position);
    temp_record = SNetTlRead(tmp_elem->stream);
    SNetRecDestroy(temp_record);
    check_position = SNetUtilListIterNext(check_position);
  }
  tmp_elem = SNetUtilListIterGet(check_position);
  temp_record = SNetTlRead(tmp_elem->stream);
  SNetTlWrite(output, temp_record);
}

/* @fn bool AllProbed(snet_util_list_t *streams)
 * @brief checks if all buffers have a probe waiting
 * @param buffers the buffers to check
 * @return true if there is a probe waiting on every buffer
 */
static bool AllProbed(snet_util_list_t *streams)
{
  bool all_probed;
  snet_util_list_iter_t *position;
  snet_collect_elem_t *current_elem;
  snet_record_t *current_record;
  bool record_is_probe;

  all_probed = true;
  position = SNetUtilListFirst(streams);
  while(SNetUtilListIterCurrentDefined(position)) {
    current_elem = SNetUtilListIterGet(position);
    position = SNetUtilListIterNext(position);
    current_record = SNetTlPeek(current_elem->stream);

    record_is_probe  = current_record != NULL &&
                  SNetRecGetDescriptor(current_record) == REC_probe;
    if(!record_is_probe) {
      all_probed = false;
      break;
    }
  }
  SNetUtilListIterDestroy(position);
  return all_probed;
}

/* true <=> recordLevel identical and record number identical or both are NULL*/
/** <!--********************************************************************-->
 *
 * @fn bool CompareSortRecords(snet_record_t *rec1, snet_record_t *rec2)
 *
 * @brief if this are two sort records, return true if both are equal
 *
 *      This compares two sort records. Two sort records are considered
 *      equal if either both are NULL or the RecordLevel and the RecordNumber
 *      is identical.
 *      If one or both of the parameter records is something else than some
 *      sort record, undefined things happen.
 *      Furthermore, this function is commutative.
 *
 * @param rec1 first record to look at
 * @param rec2 second record to look at
 * @return true if both records are equal, false otherwise
 ******************************************************************************/
static bool CompareSortRecords( snet_record_t *rec1, snet_record_t *rec2) {

  bool res;

  if( ( rec1 == NULL) || ( rec2 == NULL)) {  
    res = ( rec1 == NULL) && ( rec2 == NULL);
  }
  else {
    res = ( ( SNetRecGetLevel( rec1) == SNetRecGetLevel( rec2)) &&
            ( SNetRecGetNum( rec1) == SNetRecGetNum( rec2))); 
  }

  return( res);
}

static snet_collect_elem_t*
FindElement(snet_util_list_t *lst, snet_tl_stream_t *key) {
  snet_util_list_iter_t *current_position;
  snet_collect_elem_t *current_element;

  current_position = SNetUtilListFirst(lst);
  while(SNetUtilListIterCurrentDefined(current_position)) {
      current_element = SNetUtilListIterGet(current_position);
      current_position = SNetUtilListIterNext(current_position);
      if(current_element->stream == key) {
          break;
      }
  }
  SNetUtilListIterDestroy(current_position);
  return current_element;
}


void DeleteStream(snet_util_list_t *lst,
                  snet_tl_streamset_t *inputs,
                  snet_tl_stream_t *target_stream)
{
  snet_util_list_iter_t *current_position;
  snet_collect_elem_t *target_element;

  #ifdef COLLECTOR_DEBUG
  SNetUtilDebugNotice("(CALLINFO (COLLECTOR IDK) (DeleteStream (lst %p) (inputs %p) "
                      "(target_stream %p)))", lst,
                      inputs, target_stream);
  fprintf(stderr, "(STATEINFO (COLLECTOR IDK) (DeleteStream (lst %p) (inputs %p) "
         "(target_Stream %p)) (before deleting)", lst,
          inputs, target_stream);
  for(current_position = SNetUtilListFirst(lst);
      SNetUtilListIterCurrentDefined(current_position);
      current_position = SNetUtilListIterNext(current_position)) {
    target_element = SNetUtilListIterGet(current_position);
    fprintf(stderr, " ((stream %p) (current_elemnet %p))",
           target_element->stream,
            target_element->current);
  }
  fprintf(stderr, ")\n");
  #endif
  current_position = SNetUtilListFirst(lst);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    target_element = SNetUtilListIterGet(current_position);
    if(target_element->stream == target_stream) {
#ifdef COLLECTOR_DEBUG
      SNetUtilDebugNotice("(STATEINFO (COLLECTOR IDK) "
                        "(DeleteStream (lst %p) (inputs %p) (target_stream %p))"
                          "(at break in while loop) "
                          "(target_element (stream %p) (current_record)))",
                          lst, inputs,
                          target_stream,
                          target_element->stream,
                          target_element->current);
#endif
      break;
    }
    current_position = SNetUtilListIterNext(current_position);
  }

 if(!SNetUtilListIterCurrentDefined(current_position)) {
   SNetUtilDebugFatal("(ERROR (COLLECTOR IDK) (Stream not found in Streamset))");
 } else {
   SNetUtilListIterDelete(current_position);
   SNetMemFree(target_element);
   SNetUtilListIterDestroy(current_position);
 }
 inputs = SNetTlDeleteFromStreamset(inputs, target_stream);
}
/** <!--********************************************************************-->
 *
 * @fn void Collector(void *info)
 *
 * @brief contains mainloop of a an arbitrary collector
 *
 * @param info is some dispatch_info_t, contains all information the
 *          collector needs
 *
 ******************************************************************************/
static void *Collector( void *info) {
  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_tl_stream_t *output = inf->to_stream;
  bool is_det = inf->is_det;

  snet_record_t *rec;
  snet_util_list_t *lst = NULL;
  bool terminate = false;
  bool processed_record;
  snet_collect_elem_t *tmp_elem, *elem;

  snet_record_t *current_sort_rec = NULL;

  int counter = 0;
  int sort_end_counter = 0;

  snet_tl_streamset_t *inputs;
  snet_tl_stream_t *current_stream;
  snet_tl_stream_record_pair_t peek_result;

#ifdef COLLECTOR_DEBUG
  char record_message[100];
  SNetUtilDebugNotice("(CREATION COLLECTOR)");
#endif

  inputs = SNetTlCreateStreamset();
  SNetTlAddToStreamset(inputs, inf->from_stream);

  elem = SNetMemAlloc(sizeof(snet_collect_elem_t));
  elem->stream = inf->from_stream;
  elem->current = NULL;

  lst = SNetUtilListCreate();
  lst = SNetUtilListAddBeginning(lst, elem);

  while(!(terminate)) {
    do {
      processed_record = false;
      peek_result = SNetTlPeekMany(inputs);
      rec = peek_result.record;
      current_stream = peek_result.stream;
      elem = FindElement(lst, current_stream);

#ifdef COLLECTOR_DEBUG
      SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (start of main loop) "
                          "(reading stream %p))",
          output, current_stream);
#endif

      if( rec != NULL) {
#ifdef COLLECTOR_DEBUG
          SNetUtilDebugDumpRecord(rec, record_message);
          SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (start of main loop) "
                              "(found %s))",
                              output, 
                              record_message);
#endif

        if(CompareSortRecords(elem->current, current_sort_rec)) {
          if(SNetRecGetDescriptor(rec) != REC_probe) {
            rec = SNetTlRead(current_stream);
          }
          switch( SNetRecGetDescriptor( rec)) {
            case REC_data:
#ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (in switch) (outputting data))",
                  output);
#endif
              SNetTlWrite(output, rec);
              processed_record = true;
            break;

            case REC_sync:
              SNetTlReplaceInStreamset(inputs, current_stream,
                                          SNetRecGetStream(rec));
              processed_record = true;
            break;

            case REC_collect:
              tmp_elem = SNetMemAlloc(sizeof( snet_collect_elem_t));
              tmp_elem->stream = SNetRecGetStream(rec);

#ifdef COLLECTOR_DEBUG
              SNetUtilDebugNotice("(MODIFICATION (COLLECTOR %p) (ADD (STREAM %p)))",
                                  output, tmp_elem->stream);
#endif
              if(current_sort_rec == NULL) {
                tmp_elem->current = NULL;
              }
              else {
                tmp_elem->current = SNetRecCopy( current_sort_rec);
              }
              lst = SNetUtilListAddBeginning(lst, tmp_elem);
              SNetTlAddToStreamset(inputs, SNetRecGetStream(rec));
              SNetRecDestroy( rec);
              processed_record = true;
            break;

            case REC_sort_begin:
              elem->current = SNetRecCopy( rec);
              counter += 1;
              /* got a sort begin on all inbound streams */
              if( counter == SNetUtilListCount(lst)) {
                counter = 0;
		if(current_sort_rec != NULL) {
		  SNetRecDestroy( current_sort_rec);
		}
                current_sort_rec = SNetRecCopy( rec);
		// What's the point in this line?
		// SNetRecDestroy( current_sort_rec);
                if(is_det) {
                  if( SNetRecGetLevel( rec) > 0) {
                    SNetRecSetLevel( rec, SNetRecGetLevel( rec) - 1);
                    SNetTlWrite(output, rec);
                  } else if(SNetRecGetLevel(rec) == 0) {
                    SNetRecDestroy( rec);
                  } else {
#ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("(ERROR (COLLECTOR %p) (record "
                                        "with level < 0!))", output);
#endif
                    SNetRecDestroy(rec);
                  }
                } else {
#ifdef COLLECTOR_DEBUG
                   SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (in switch) (found enough sort"
                      "records outputing sort stuff))", output);
#endif
                  SNetTlWrite(output, rec);
                }
              } else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            break;

            case REC_sort_end:
              sort_end_counter += 1;
              if(sort_end_counter == SNetUtilListCount(lst)) {
                sort_end_counter = 0;
                if(is_det) {
                  if(SNetRecGetLevel(rec) > 0) {
                    SNetRecSetLevel(rec, SNetRecGetLevel( rec) - 1);
                    SNetTlWrite(output, rec);
                  } else if(SNetRecGetLevel(rec) == 0) {
                    SNetRecDestroy(rec);
                  } else {
#ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("(ERROR (COLLECTOR %p) (found a sort end record))"
                          "with level < 0.");
#endif
                    SNetRecDestroy(rec);
                  }
                } else {
#ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (in switch) "
                                        "(found enoguh sort end records, outputtting things))", 
                                        output);
#endif
                  SNetTlWrite(output, rec);
                }
              } else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            break;

            case REC_terminate:
#ifdef COLLECTOR_DEBUG
              SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (in switch, before DeleteStream) "
                                  "(%d buffers left before deletion))",
                                  output, SNetUtilListCount(lst));
#endif
              DeleteStream(lst, inputs, current_stream);
#ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) "
                                    "(in switch, after DeleteStream) (%d buffers left))",
                                    output, SNetUtilListCount(lst));
#endif
              if(SNetUtilListIsEmpty(lst)) {
                terminate = true;
                SNetTlWrite(output, rec);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;

            break;

            case REC_probe:
              if(AllProbed(lst)) {
                RemoveProbes(lst, output);
              }
            break;
	  default:
	    SNetUtilDebugNotice("[Collector] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
	    SNetRecDestroy( rec);
	    break;
          } // switch
        } // if sort record fits
#ifdef COLLECTOR_DEBUG 
          else {
            SNetUtilDebugNotice("(STATEINFO (COLLECTOR %p) (in some else far below)"
                                "(rejected due to sort record))",
                                output);
          }
#endif
      } // if
    } while( processed_record && !(terminate));
  } // while !terminate

  SNetTlMarkObsolete(output);
  SNetUtilListDestroy(lst);
  SNetTlDestroyStreamset(inputs);
  SNetMemFree( inf);
  return( NULL);
}


static snet_tl_stream_t*
CollectorStartup(snet_tl_stream_t *initial_stream, bool is_det)
{
  snet_tl_stream_t *output;
  dispatch_info_t *streams;

  streams = SNetMemAlloc( sizeof( dispatch_info_t));
  output = SNetTlCreateStream(BUFFER_SIZE);

  streams->from_stream = initial_stream;
  streams->to_stream = output;

#ifdef COLLECTOR_DEBUG
  SNetUtilDebugNotice("-");
  if(is_det) {
    SNetUtilDebugNotice("| DET COLLECTOR CREATED");
  } else {
    SNetUtilDebugNotice("| COLLECTOR CREATED");
  }
  SNetUtilDebugNotice("| input: %p", initial_stream);
  SNetUtilDebugNotice("| output: %p", output);
  SNetUtilDebugNotice("-");
#endif

  streams->is_det = is_det;
  if( is_det) {
    SNetThreadCreate( Collector, (void*)streams, ENTITY_collect_det);
  }
  else {
    SNetThreadCreate( Collector, (void*)streams, ENTITY_collect_nondet);
  }

  return( output);
}

snet_tl_stream_t *CreateCollector( snet_tl_stream_t *initial_stream)
{
  return( CollectorStartup( initial_stream, false));
}

snet_tl_stream_t *CreateDetCollector(snet_tl_stream_t *initial_stream)
{
  return( CollectorStartup( initial_stream, true));
}

