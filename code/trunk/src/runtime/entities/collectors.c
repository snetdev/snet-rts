#include <stdlib.h>

#include "collectors.h"
#include "memfun.h"
#include "handle.h"
#include "buffer.h"
#include "record.h"
#include "threading.h"
#include "bool.h"
#include "list.h"
#include "debug.h"

/* #define COLLECTOR_DEBUG */
typedef struct {
  snet_buffer_t *from_buf;
  snet_buffer_t *to_buf;
  char *desc;

  bool is_det;
} dispatch_info_t;

typedef struct {
  snet_buffer_t *buf;
  snet_record_t *current;
} snet_collect_elem_t;


/* @fn void RemoveProbes(snet_util_list_t *buffers, snet_buffer_t *outbuf)
 * @brief removes the probes on all buffers in the list and passes one
 *        to the outer world
 * @param buffers the buffers to remove a probe on
 * @param outbuf the outbuf to send a probe to
 */
static void RemoveProbes(snet_util_list_t *buffers, snet_buffer_t *outbuf) {
  snet_util_list_iter_t *check_position;
  snet_collect_elem_t *tmp_elem;
  snet_record_t *temp_record;

  check_position = SNetUtilListFirst(buffers);
  while(SNetUtilListIterHasNext(check_position)) {
    tmp_elem = SNetUtilListIterGet(check_position);
    temp_record = SNetBufGet(tmp_elem->buf);
    SNetRecDestroy(temp_record);
    check_position = SNetUtilListIterNext(check_position);
  }
  tmp_elem = SNetUtilListIterGet(check_position);
  temp_record = SNetBufGet(tmp_elem->buf);
  SNetBufPut(outbuf, temp_record);
}

/* @fn bool AllProbed(snet_util_list_t *buffers)
 * @brief checks if all buffers have a probe waiting
 * @param buffers the buffers to check
 * @return true if there is a probe waiting on every buffer
 */
static bool AllProbed(snet_util_list_t *buffers) {
  bool all_probed;
  snet_util_list_iter_t *position;
  snet_collect_elem_t *current_elem;
  snet_record_t *current_record;
  bool record_is_probe;

  all_probed = true;
  position = SNetUtilListFirst(buffers);
  while(SNetUtilListIterCurrentDefined(position)) {
    current_elem = SNetUtilListIterGet(position);
    position = SNetUtilListIterNext(position);
    current_record = SNetBufShow(current_elem->buf);

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
  int i,j;

  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  bool is_det = inf->is_det;

  snet_buffer_t *current_buf;
  sem_t *sem;
  snet_record_t *rec;
  snet_util_list_t *lst = NULL;
  bool terminate = false;
  bool processed_record;
  snet_collect_elem_t *tmp_elem, *elem;

  snet_record_t *current_sort_rec = NULL;

  int counter = 0;
  int sort_end_counter = 0;

  snet_util_list_iter_t *read_position;

  sem = SNetMemAlloc(sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher(inf->from_buf, sem);

  elem = SNetMemAlloc(sizeof(snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = SNetUtilListCreate();
  lst = SNetUtilListAddBeginning(lst, elem);
  read_position = SNetUtilListFirst(lst);

  while(!(terminate)) {
    sem_wait(sem);
    do {
      processed_record = false;
      j = SNetUtilListCount(lst);
      for( i=0; (i<j) && !(terminate); i++) {
        elem = (snet_collect_elem_t*) SNetUtilListIterGet(read_position);
        current_buf = elem->buf;

        #ifdef COLLECTOR_DEBUG
          SNetUtilDebugNotice("COLLECTOR %x: reading buffer %x",
            (unsigned int) outbuf, (unsigned int) current_buf);
        #endif

        rec = SNetBufShow(current_buf);
        if( rec != NULL) {
          #ifdef COLLECTOR_DEBUG
            SNetUtilDebugNotice("COLLECTOR %x: found data",
              (unsigned int) outbuf);
          #endif

          if(CompareSortRecords(elem->current, current_sort_rec)) {
            if(SNetRecGetDescriptor(rec) != REC_probe) {
              rec = SNetBufGet(elem->buf);
            }
            switch( SNetRecGetDescriptor( rec)) {
              case REC_data:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: Data record", (
                    unsigned int) outbuf);
                  SNetUtilDebugNotice("COLLECTOR %x: outputting data",
                    (unsigned int) outbuf);
                #endif
                SNetBufPut( outbuf, rec);
                processed_record = true;
              break;

              case REC_sync:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: sync record",
                    (unsigned int) outbuf);
                #endif
                tmp_elem = SNetUtilListIterGet(read_position);
                tmp_elem->buf = SNetRecGetBuffer( rec);
                lst = SNetUtilListIterSet(read_position, tmp_elem);

                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record = true;
              break;

              case REC_collect:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: collect record",
                    (unsigned int) outbuf);
                #endif

                tmp_elem = SNetMemAlloc(sizeof( snet_collect_elem_t));
                tmp_elem->buf = SNetRecGetBuffer(rec);

                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: adding buffer %x",
                    (unsigned int) outbuf, (unsigned int) tmp_elem->buf);
                #endif
                if(current_sort_rec == NULL) {
                  tmp_elem->current = NULL;
                }
                else {
                  tmp_elem->current = SNetRecCopy( current_sort_rec);
                }
                lst = SNetUtilListAddEnd(lst, tmp_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record = true;
              break;

              case REC_sort_begin:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: sort_begin record",
                    (unsigned int) outbuf);
                #endif
                elem->current = SNetRecCopy( rec);
                counter += 1;
                /* got a sort begin on all inbound streams */
                if( counter == SNetUtilListCount(lst)) {
                  counter = 0;
                  current_sort_rec = SNetRecCopy( rec);
                  SNetRecDestroy( current_sort_rec);
                  if(is_det) {
                    if( SNetRecGetLevel( rec) > 0) {
                      SNetRecSetLevel( rec, SNetRecGetLevel( rec) - 1);
                      SNetBufPut( outbuf, rec);
                    } else if(SNetRecGetLevel(rec) == 0) {
                      SNetRecDestroy( rec);
                    } else {
                      SNetUtilDebugNotice("COLLECTOR: found a sort record"
                        "with level < 0!");
                      SNetRecDestroy(rec);
                    }
                  } else {
                    #ifdef COLLECTOR_DEBUG
                     SNetUtilDebugNotice("COLLECTOR %x: found enough sort"
                        "records outputing sort stuff", (unsigned int) outbuf);
                    #endif
                    SNetBufPut(outbuf, rec);
                  }
                } else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              break;

              case REC_sort_end:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR: %x sort_end record",
                    (unsigned int) outbuf);
                #endif
                sort_end_counter += 1;
                if(sort_end_counter == SNetUtilListCount(lst)) {
                  sort_end_counter = 0;
                  if(is_det) {
                    if(SNetRecGetLevel(rec) > 0) {
                      SNetRecSetLevel(rec, SNetRecGetLevel( rec) - 1);
                      SNetBufPut(outbuf, rec);
                    } else if(SNetRecGetLevel(rec) == 0) {
                      SNetRecDestroy(rec);
                    } else {
                      SNetUtilDebugNotice("COLLECTOR: found a sort end record"
                            "with level < 0.");
                      SNetRecDestroy(rec);
                    }
                  } else {
                    #ifdef COLLECTOR_DEBUG
                      SNetUtilDebugNotice("COLLECTOR: found enoguh sort end"
                        " records, outputtting things");
                    #endif
                    SNetBufPut(outbuf, rec);
                  }
                } else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              break;

              case REC_terminate:
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: terminate record",
                    (unsigned int) outbuf);
                #endif
                tmp_elem = SNetUtilListIterGet(read_position);
                SNetBufDestroy( tmp_elem->buf);
                lst = SNetUtilListIterDelete(read_position);
                if(!SNetUtilListIterCurrentDefined(read_position)) {
                    /* ouch! expensive goto */
                    SNetUtilListIterDestroy(read_position);
                    read_position = SNetUtilListFirst(lst);
                }
                SNetMemFree(tmp_elem);
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: %d buffers left",
                    (unsigned int) outbuf, SNetUtilListCount(lst));
                #endif
                if(SNetUtilListIsEmpty(lst)) {
                  terminate = true;
                  #ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("COLLECTOR %x: forwarding TERMINATE"
                      " token", (unsigned int) outbuf);
                  #endif
                  SNetBufPut(outbuf, rec);
                }
                else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;

              break;

              case REC_probe:
                SNetUtilDebugNotice("probing in collector");
                if(AllProbed(lst)) {
                  RemoveProbes(lst, outbuf);
                }
              break;
            } // switch
          } // if sort-record is right
          else {
            read_position = SNetUtilListIterRotateForward(read_position);
          }
        } // if
        else {
          #ifdef COLLECTOR_DEBUG
            SNetUtilDebugNotice("COLLECTOR %x: no data.",
              (unsigned int) outbuf);
          #endif
          read_position  = SNetUtilListIterRotateForward(read_position);
        }
      } // for getCount
    } while( processed_record && !(terminate));
  } // while !terminate

  SNetBufBlockUntilEmpty(outbuf);
  SNetBufDestroy(outbuf);
  SNetUtilListDestroy(lst);
  SNetUtilListIterDestroy(read_position);
  SNetMemFree( inf);
  return( NULL);
}


static snet_buffer_t *CollectorStartup( snet_buffer_t *initial_buffer, 
                                        bool is_det) {
  snet_buffer_t *outbuf;
  dispatch_info_t *buffers;

  buffers = SNetMemAlloc( sizeof( dispatch_info_t));
  outbuf = SNetBufCreate( BUFFER_SIZE);

  buffers->from_buf = initial_buffer;
  buffers->to_buf = outbuf;

  buffers->is_det = is_det;
  if( is_det) {
    SNetThreadCreate( Collector, (void*)buffers, ENTITY_collect_det);
  }
  else {
    SNetThreadCreate( Collector, (void*)buffers, ENTITY_collect_nondet);
  }

  return( outbuf);
}

snet_buffer_t *CreateCollector( snet_buffer_t *initial_buffer) {	

  return( CollectorStartup( initial_buffer, false));
}

snet_buffer_t *CreateDetCollector( snet_buffer_t *initial_buffer) { 

  return( CollectorStartup( initial_buffer, true));
}

