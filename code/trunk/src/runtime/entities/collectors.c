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
 * @fn void Detcollector(void *info) 
 *
 * @brief contains mainloop of a deterministic collector
 *
 * @param info is some dispatch_info_t, contains all information the 
 *          collector needs
 *
 ******************************************************************************/
static void *DetCollector( void *info) {
  int i, j;
  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  sem_t *sem;
  snet_record_t *rec;
  snet_util_list_t *lst = NULL; 
  bool terminate = false;
  bool got_record;

  bool all_probed;
  snet_record_t *temp_record;

  snet_collect_elem_t *tmp_elem, *new_elem, *elem;

  snet_record_t *current_sort_rec = NULL;
  
  int counter = 0; 
  int sort_end_counter = 0;

  bool processed_record;

  snet_util_list_iter_t *read_position;
  snet_util_list_iter_t *check_position;
  sem = SNetMemAlloc( sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher( inf->from_buf, sem);
  
  elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = SNetUtilListCreate();
  lst = SNetUtilListAddBeginning(lst, elem);
  read_position = SNetUtilListFirst(lst);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
      processed_record = false;
      
      i = 0;
      j = SNetUtilListCount(lst);
      for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
        tmp_elem = SNetUtilListIterGet(read_position);
        rec = SNetBufShow( tmp_elem->buf);
        if( rec != NULL) { 
         
          got_record = true;
          
          switch(SNetRecGetDescriptor( rec)) {
            case REC_data:
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet( tmp_elem->buf);
                SNetBufPut( outbuf, rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
                  
//            rec = SNetBufGet( current_buf);
//            SNetBufPut( outbuf, rec);
              break;
    
            case REC_sync:
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem->buf = SNetRecGetBuffer( rec);
                SNetUtilListIterSet(read_position, tmp_elem);
                SNetBufRegisterDispatcher(SNetRecGetBuffer( rec), sem);
                SNetRecDestroy(rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
    
            case REC_collect:
              tmp_elem = SNetUtilListIterGet(read_position); 
              if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                new_elem = SNetMemAlloc(sizeof( snet_collect_elem_t));
                new_elem->buf = SNetRecGetBuffer(rec);
                if( current_sort_rec == NULL) {
                  new_elem->current = NULL;
                }
                else {
                   new_elem->current = SNetRecCopy(current_sort_rec);
                }
                lst = SNetUtilListAddEnd(lst, new_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position); 
              }            
              break;
            case REC_sort_begin:
              tmp_elem = SNetUtilListIterGet(read_position); 
              if( CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem->current = SNetRecCopy( rec);
                counter += 1;

                if( counter == SNetUtilListCount(lst)) { 
                  counter = 0;
                  current_sort_rec = SNetRecCopy( rec);

                  if( SNetRecGetLevel( rec) != 0) {
                    SNetRecSetLevel( rec, SNetRecGetLevel( rec) - 1);
                    SNetBufPut( outbuf, rec);
                  }
                  else {
                     SNetRecDestroy( rec);
                  }
                }
                else {
                   SNetRecDestroy( rec);
                }
                processed_record=true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
            case REC_sort_end:
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet( tmp_elem->buf);
                sort_end_counter += 1;

                if(sort_end_counter == SNetUtilListCount(lst)) {
                  sort_end_counter = 0;

                  if( SNetRecGetLevel(rec) != 0) {
                    SNetRecSetLevel(rec, SNetRecGetLevel( rec) - 1);
                    SNetBufPut(outbuf, rec);
                  }
                  else {
                    SNetRecDestroy(rec);
                  }
                }
                else {
                  SNetRecDestroy(rec);
                }
                processed_record=true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
            case REC_terminate:
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetUtilListIterGet(read_position);
                SNetBufDestroy(tmp_elem->buf);
                lst = SNetUtilListIterDelete(read_position);
                if(!SNetUtilListIterCurrentDefined(read_position)) {
                  /* ouch! expensive goto */
                  SNetUtilListIterDestroy(read_position);
                  read_position = SNetUtilListFirst(lst);
                }
                SNetMemFree(tmp_elem);
                if(SNetUtilListIsEmpty(lst)) {
                  terminate = true;
                  SNetBufPut( outbuf, rec);
                  SNetBufBlockUntilEmpty( outbuf);
                  SNetBufDestroy( outbuf);
                  SNetUtilListDestroy( lst);
                  SNetUtilListIterDestroy(read_position);
                }
                else {
                  SNetRecDestroy(rec);
                }
                processed_record=true;
              }
              else {
//              sem_post( sem);
                SNetUtilListIterRotateForward(read_position);
              }

              break;

            case REC_probe:
              SNetUtilDebugNotice("collector probed");
              /* we are only allowed to foward a probe record if this record
               * type is present on all input streams.
               */
              check_position = SNetUtilListFirst(lst);
              all_probed = true;
              while(SNetUtilListIterCurrentDefined(check_position)) {
                tmp_elem = SNetUtilListIterGet(check_position);
                temp_record = SNetBufShow(tmp_elem->buf);
                if(temp_record == NULL ||
                   SNetRecGetDescriptor(temp_record) != REC_probe) {
                  all_probed = false;
                  break;
                }
                check_position = SNetUtilListIterNext(check_position);
              }
              SNetUtilListIterDestroy(check_position);
              if(all_probed) {
                /* all input buffers are probed, lets remove those
                 * probes! (and pass one to the outer world)
                 */
                check_position = SNetUtilListFirst(lst);
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
              SNetUtilDebugNotice("collector probing done");
            break;
          } // switch
        } // if
        else {
          read_position = SNetUtilListIterRotateForward(read_position);
        }
  //    } // while !got_record
        read_position = SNetUtilListIterRotateForward(read_position);
        i++;
      } // for
    } while( processed_record && !(terminate));
  } // while !terminate

  SNetMemFree( inf);
  return( NULL);
}

static void *Collector( void *info) {
  int i,j;

  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  snet_buffer_t *current_buf;
  sem_t *sem;
  snet_record_t *rec;
  snet_util_list_t *lst = NULL;
  bool terminate = false;
  bool got_record;
  bool processed_record;
  snet_collect_elem_t *tmp_elem, *elem;

  snet_record_t *current_sort_rec = NULL;
  
  int counter = 0; 
  int sort_end_counter = 0;
  bool all_probed;
  snet_record_t *temp_record;

  snet_util_list_iter_t *read_position;
  snet_util_list_iter_t *temp_position;

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
    got_record = false;
    sem_wait(sem);
    do {
      got_record = false;
      processed_record = false;
      j = SNetUtilListCount(lst);
      for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
        current_buf = ((snet_collect_elem_t*) SNetUtilListIterGet(read_position))->buf;
        #ifdef COLLECTOR_DEBUG
          SNetUtilDebugNotice("COLLECTOR %x: reading buffer %x",
            (unsigned int) outbuf, (unsigned int) current_buf);
        #endif
        rec = SNetBufShow(current_buf);
        if( rec != NULL) {
          got_record = true;
          #ifdef COLLECTOR_DEBUG
            SNetUtilDebugNotice("COLLECTOR %x: found data",
              (unsigned int) outbuf);
          #endif
          switch( SNetRecGetDescriptor( rec)) {
            case REC_data:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR %x: Data record", (
                  unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                #ifdef COLLECTOR_DEBUG
                  SNetUtilDebugNotice("COLLECTOR %x: outputting data",
                    (unsigned int) outbuf);
                #endif
                rec = SNetBufGet( tmp_elem->buf);
                SNetBufPut( outbuf, rec);
                processed_record = true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
                  
              //rec = SNetBufGet( current_buf);
              //SNetBufPut( outbuf, rec);
              break;
    
            case REC_sync:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR %x: sync record",
                  (unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if( CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetUtilListIterGet(read_position);
                tmp_elem->buf = SNetRecGetBuffer( rec);
                SNetUtilListIterSet(read_position, tmp_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record = true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;

            case REC_collect:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR %x: collect record", 
                  (unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
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
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
            case REC_sort_begin:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR %x: sort_begin record",
                  (unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem->current = SNetRecCopy( rec);
                counter += 1;

                if(counter == SNetUtilListCount(lst)) {
                  SNetRecDestroy( current_sort_rec);
                  current_sort_rec = SNetRecCopy( rec);
                  counter = 0;
                  #ifdef COLLECTOR_DEBUG
                   SNetUtilDebugNotice("COLLECTOR %x: found enough sort records"
                      " outputing sort stuff", (unsigned int) outbuf);
                  #endif
                  SNetBufPut(outbuf, rec);
                }
                else {
                  SNetRecDestroy(rec);
                }
                processed_record = true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
            case REC_sort_end:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR: %x sort_end record", (unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                sort_end_counter += 1;
                if( sort_end_counter == SNetUtilListCount(lst)) {
                  #ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("COLLECTOR: found enoguh sort end"
                      " records, outputtting things");
                  #endif
                  SNetBufPut(outbuf, rec);
                  sort_end_counter = 0;
                }
                else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              }
              else {
                read_position = SNetUtilListIterRotateForward(read_position);
              }
              break;
            case REC_terminate:
              #ifdef COLLECTOR_DEBUG
                SNetUtilDebugNotice("COLLECTOR %x: terminate record",
                  (unsigned int) outbuf);
              #endif
              tmp_elem = SNetUtilListIterGet(read_position);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
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
                if(SNetUtilListCount(lst) == 0) {
                  terminate = true;
                  #ifdef COLLECTOR_DEBUG
                    SNetUtilDebugNotice("COLLECTOR %x: forwarding TERMINATE"
                      " token", (unsigned int) outbuf);
                  #endif
                  SNetBufPut(outbuf, rec);
                  SNetBufBlockUntilEmpty(outbuf);
                  SNetBufDestroy(outbuf);
                  SNetUtilListDestroy(lst);
                  SNetUtilListIterDestroy(read_position);
                }
                else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              }
              else {
//              sem_post( sem);
                read_position = SNetUtilListIterRotateForward(read_position);
              }

              break;
            case REC_probe:
              SNetUtilDebugNotice("probing in collector");
              /* we are only allowed to foward a probe record if this record
               * type is present on all input streams.
               */
              all_probed = true;
              temp_position = SNetUtilListFirst(lst);
              while(SNetUtilListIterCurrentDefined(temp_position)) {
                tmp_elem = SNetUtilListIterGet(temp_position);
                temp_position = SNetUtilListIterNext(temp_position);
                temp_record = SNetBufShow(tmp_elem->buf);
                if(temp_record == NULL ||
                   SNetRecGetDescriptor(temp_record) != REC_probe) {
                  all_probed = false;
                  break; /* for */
                }
              }
              SNetUtilListIterDestroy(temp_position);

              SNetUtilDebugNotice("%d", all_probed);
              if(all_probed) {
                /* all input buffers are probed, lets remove those
                 * probes! (and pass one to the outer world)
                 */
                temp_position = SNetUtilListFirst(lst);
                while(SNetUtilListIterCurrentDefined(temp_position)) {
                  tmp_elem = SNetUtilListIterGet(temp_position);
                  temp_position = SNetUtilListIterNext(temp_position);
                  temp_record = SNetBufGet(tmp_elem->buf);
                  SNetRecDestroy(temp_record);
                }
                tmp_elem = SNetUtilListIterGet(temp_position);
                temp_record = SNetBufGet(tmp_elem->buf);
                SNetBufPut(outbuf, temp_record);
                SNetUtilListIterDestroy(temp_position);
                SNetUtilDebugNotice("removed all probes");
              }
            break;
          } // switch
        } // if
        else {
          #ifdef COLLECTOR_DEBUG
            SNetUtilDebugNotice("COLLECTOR %x: no data.",
              (unsigned int) outbuf);
          #endif
          read_position  = SNetUtilListIterRotateForward(read_position);
        }
  //    } // while !got_record
      } // for getCount
    } while( processed_record && !(terminate));
  } // while !terminate

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


  if( is_det) {
    SNetThreadCreate( DetCollector, (void*)buffers, ENTITY_collect_det);
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

