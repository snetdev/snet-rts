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

  sem = SNetMemAlloc( sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher( inf->from_buf, sem);
  
  elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = SNetUtilListCreate();
  SNetUtilListAddBeginning(lst, elem);
  SNetUtilListGotoBeginning(lst);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
      processed_record = false;
      
      i = 0;
      j = SNetUtilListCount(lst);
      for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
        tmp_elem = SNetUtilListGet(lst);
        rec = SNetBufShow( tmp_elem->buf);
        if( rec != NULL) { 
         
          got_record = true;
          
          switch(SNetRecGetDescriptor( rec)) {
            case REC_data:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet( tmp_elem->buf);
                SNetBufPut( outbuf, rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }
                  
//            rec = SNetBufGet( current_buf);
//            SNetBufPut( outbuf, rec);
              break;
    
            case REC_sync:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords( tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem->buf = SNetRecGetBuffer( rec);
                SNetUtilListSet(lst, tmp_elem); 
                SNetBufRegisterDispatcher(SNetRecGetBuffer( rec), sem);
                SNetRecDestroy(rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst); 
              }
              break;
    
            case REC_collect:
              tmp_elem = SNetUtilListGet(lst); 
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
                SNetUtilListAddEnd(lst, new_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record=true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst); 
              }            
              break;
            case REC_sort_begin:
              tmp_elem = SNetUtilListGet(lst); 
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
                SNetUtilListRotateForward(lst); 
              }
              break;
            case REC_sort_end:
              tmp_elem = SNetUtilListGet(lst); 
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
                SNetUtilListRotateForward(lst);
              }
              break;
            case REC_terminate:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetUtilListGet(lst);
                SNetBufDestroy(tmp_elem->buf);
                SNetUtilListDelete(lst);
                SNetMemFree(tmp_elem);
                if(SNetUtilListIsEmpty(lst)) {
                  terminate = true;
                  SNetBufPut( outbuf, rec);
                  SNetBufBlockUntilEmpty( outbuf);
                  SNetBufDestroy( outbuf);
                  SNetUtilListDestroy( lst);
                }
                else {
                  SNetRecDestroy(rec);
                }
                processed_record=true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }

              break;

            case REC_probe:
              SNetUtilDebugNotice("collector probed");
              /* we are only allowed to foward a probe record if this record
               * type is present on all input streams.
               */
              all_probed = true;
              for(i = 0; i < SNetUtilListCount(lst); i++) {
                /* This is some tiny optimization. If there was a stream
                 * without a probe record on it already, then we dont
                 * need to look at the other buffers. However, we have
                 * to keep rotation so we do not modify the current-element
                 * of the list (because it loops around, it will end up at
                 * the start again)
                 */
                if(all_probed) {
                  tmp_elem = SNetUtilListGet(lst);
                  temp_record = SNetBufShow(tmp_elem->buf);
                  if(temp_record == NULL ||
                     SNetRecGetDescriptor(temp_record) != REC_probe) {
                    all_probed = false;
                  }
                }
                SNetUtilListRotateForward(lst);
              }
              if(all_probed) {
                /* all input buffers are probed, lets remove those
                 * probes! (and pass one to the outer world)
                 */
                for(i = 0; i < SNetUtilListCount(lst)-1; i++) {
                  tmp_elem = SNetUtilListGet(lst);
                  temp_record = SNetBufGet(tmp_elem->buf);
                  SNetRecDestroy(temp_record);
                  SNetUtilListRotateForward(lst);
                }
                tmp_elem = SNetUtilListGet(lst);
                temp_record = SNetBufGet(tmp_elem->buf);
                SNetBufPut(outbuf, temp_record);
                SNetUtilListRotateForward(lst);
              }
              SNetUtilDebugNotice("collector probing done");
            break;
          } // switch
        } // if
        else {
          SNetUtilListRotateForward(lst);
        }
  //    } // while !got_record
        SNetUtilListRotateForward(lst);
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

  sem = SNetMemAlloc(sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher(inf->from_buf, sem);
  
  elem = SNetMemAlloc(sizeof(snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = SNetUtilListCreate();
  SNetUtilListAddBeginning(lst, elem);
  SNetUtilListGotoBeginning(lst);

  while(!(terminate)) {
    got_record = false;
    sem_wait(sem);
    do {
      got_record = false;
      processed_record = false;
      j = SNetUtilListCount(lst);
      for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
        //Segfault here.
        current_buf = ((snet_collect_elem_t*) SNetUtilListGet(lst))->buf;
        rec = SNetBufShow(current_buf);
        if( rec != NULL) { 
          got_record = true;
          
          switch( SNetRecGetDescriptor( rec)) {
            case REC_data:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                SNetUtilDebugNotice("moving data");
                rec = SNetBufGet( tmp_elem->buf);
                SNetBufPut( outbuf, rec);
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }
                  
              //rec = SNetBufGet( current_buf);
              //SNetBufPut( outbuf, rec);
              break;
    
            case REC_sync:
              tmp_elem = SNetUtilListGet(lst);
              if( CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetUtilListGet(lst);
                tmp_elem->buf = SNetRecGetBuffer( rec);
                SNetUtilListSet(lst, tmp_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }
              break;
    
            case REC_collect:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetMemAlloc(sizeof( snet_collect_elem_t));
                tmp_elem->buf = SNetRecGetBuffer(rec);
                if(current_sort_rec == NULL) {
                  tmp_elem->current = NULL;
                }
                else {
                  tmp_elem->current = SNetRecCopy( current_sort_rec);
                }
                SNetUtilListAddEnd(lst, tmp_elem);
                SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
                SNetRecDestroy( rec);
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }            
              break;
            case REC_sort_begin:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem->current = SNetRecCopy( rec);
                counter += 1;

                if(counter == SNetUtilListCount(lst)) {
                  SNetRecDestroy( current_sort_rec);
                  current_sort_rec = SNetRecCopy( rec);
                  counter = 0;
                  SNetBufPut(outbuf, rec);
                }
                else {
                  SNetRecDestroy(rec);
                }
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }
              break;
            case REC_sort_end:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                sort_end_counter += 1;
                if( sort_end_counter == SNetUtilListCount(lst)) {
                  SNetBufPut(outbuf, rec);
                  sort_end_counter = 0;
                }
                else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward( lst);
              }
              break;
            case REC_terminate:
              tmp_elem = SNetUtilListGet(lst);
              if(CompareSortRecords(tmp_elem->current, current_sort_rec)) {
                rec = SNetBufGet(tmp_elem->buf);
                tmp_elem = SNetUtilListGet(lst);
                SNetBufDestroy( tmp_elem->buf);
                SNetUtilListDelete(lst);
                if(!SNetUtilListCurrentDefined(lst)) {
                  SNetUtilListGotoBeginning(lst);
                }
                SNetMemFree(tmp_elem);
                if(SNetUtilListCount(lst) == 0) {
                  terminate = true;
                  SNetBufPut(outbuf, rec);
                  SNetBufBlockUntilEmpty(outbuf);
                  SNetBufDestroy(outbuf);
                  SNetUtilListDestroy(lst);
                }
                else {
                  SNetRecDestroy( rec);
                }
                processed_record = true;
              }
              else {
//              sem_post( sem);
                SNetUtilListRotateForward(lst);
              }

              break;
            case REC_probe:
              SNetUtilDebugNotice("probing in collector");
              /* we are only allowed to foward a probe record if this record
               * type is present on all input streams.
               */
              all_probed = true;
              for(i = 0; i < SNetUtilListCount(lst); i++) {
                /* This is some tiny optimization. If there was a stream
                 * without a probe record on it already, then we dont
                 * need to look at the other buffers. However, we have
                 * to keep rotation so we do not modify the current-element
                 * of the list (because it loops around, it will end up at
                 * the start again)
                 */
                if(all_probed) {
                  tmp_elem = SNetUtilListGet(lst);
                  temp_record = SNetBufShow(tmp_elem->buf);
                  if(temp_record == NULL ||
                     SNetRecGetDescriptor(temp_record) != REC_probe) {
                    SNetUtilDebugNotice("fail at %d with %d", i, 
                                          (temp_record == NULL? -1 : SNetRecGetDescriptor(temp_record)));
                    all_probed = false;
                    break; /* for */
                  }
                }
                SNetUtilListRotateForward(lst);
              }
              SNetUtilDebugNotice("%d", all_probed);
              if(all_probed) {
                /* all input buffers are probed, lets remove those
                 * probes! (and pass one to the outer world)
                 */
                for(i = 0; i < SNetUtilListCount(lst)-1; i++) {
                  tmp_elem = SNetUtilListGet(lst);
                  temp_record = SNetBufGet(tmp_elem->buf);
                  SNetRecDestroy(temp_record);
                  SNetUtilListRotateForward(lst);
                }
                tmp_elem = SNetUtilListGet(lst);
                temp_record = SNetBufGet(tmp_elem->buf);
                SNetBufPut(outbuf, temp_record);
                SNetUtilListRotateForward(lst);
                SNetUtilDebugNotice("removed all probes");
              }
            break;
          } // switch
        } // if
        else {
          SNetUtilListRotateForward(lst);
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

