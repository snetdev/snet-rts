#include "collectors.h"
#include "memfun.h"
#include "handle.h"
#include "buffer.h"
#include "record.h"
#include "threading.h"
#include "stdlib.h" /* for LIST */
#include "stdio.h" /* for LIST */
/* hkr: TODO: remove this list, use SNetUtilList. */
/* ========================================================================= */
/* double linked list ------------------------------------------------------ */

typedef struct {
  void *elem;
  void *prev; 
  void *next;
} list_elem_t;

typedef struct {
  list_elem_t *head;
  list_elem_t *curr;
  list_elem_t *last;
  int elem_count;
} linked_list_t;

static linked_list_t *LLSTcreate( void *first_item) {
  linked_list_t *lst;
  list_elem_t *lst_elem;

  lst_elem = malloc( sizeof( list_elem_t));
  lst_elem->elem = first_item;
  lst_elem->prev = NULL;
  lst_elem->next = NULL;

  lst = malloc( sizeof( linked_list_t));
  lst->elem_count = 1;

  lst->head = lst_elem;
  lst->curr = lst_elem;
  lst->last = lst_elem;
  lst->head->prev = NULL;
  lst->head->prev = NULL;

  return( lst);
}

static void *LLSTget( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    return( lst->curr->elem);
  }
  else {
    fprintf( stderr, "\nWarning, returning NULL (list empty)\n\n");
    return( NULL);
  }
}

static void LLSTset( linked_list_t *lst, void *elem) {
  if( lst->elem_count > 0) {
    lst->curr->elem = elem;
  }
}

static void LLSTnext( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    lst->curr = ( lst->curr->next == NULL) ? lst->head : lst->curr->next;
  }
}

/*
static void LLSTprev( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    lst->curr = (lst->curr->prev == NULL) ? lst->last : lst->curr->prev;
  }
}
*/

static void LLSTadd( linked_list_t *lst, void *elem) {
  list_elem_t *new_elem;

  new_elem = malloc( sizeof( list_elem_t));
  new_elem->elem = elem;
  new_elem->next = NULL;
  new_elem->prev = lst->last;
  
  if( lst->elem_count == 0) {
    lst->head = new_elem;
    lst->curr = new_elem;
  }
  else {
    lst->last->next = new_elem;
  }
  lst->last = new_elem;

  lst->elem_count += 1;
}


static void LLSTremoveCurrent( linked_list_t *lst) {

  list_elem_t *tmp;

  if( lst->elem_count == 0) {
    fprintf( stderr, 
             "\n\n ** Fatal Error ** : Can't delete from empty list.\n\n");
    exit( 1);
  }

  // just one element left
  if( lst->head == lst->last) {
    SNetMemFree( lst->curr);
    lst->head = NULL;
    lst->curr = NULL;
    lst->last = NULL;
  }
  else {
    // deleting head
    if( lst->curr == lst->head) {
      ((list_elem_t*)lst->curr->next)->prev = NULL;
      tmp = lst->curr;
      lst->head = lst->curr->next;
      lst->curr = lst->curr->next;
      SNetMemFree( tmp);
    } 
    else {
      // deleting last element
      if( lst->curr == lst->last) {
        ((list_elem_t*)lst->curr->prev)->next = NULL;
        tmp = lst->curr;
        lst->last = lst->curr->prev;
        lst->curr = lst->curr->prev;
        SNetMemFree( tmp);
      }
      // delete ordinary element
      else {
      ((list_elem_t*)lst->curr->next)->prev = lst->curr->prev;  
      ((list_elem_t*)lst->curr->prev)->next = lst->curr->next;
      tmp = lst->curr;
      lst->curr = lst->curr->prev;
      SNetMemFree( tmp);
      }
    } 
  }
  lst->elem_count -= 1;
} 

static int LLSTgetCount( linked_list_t *lst) {
  if( lst != NULL) {
    return( lst->elem_count);
  }
  else {
    return( -1);
  }
}

static void LLSTdestroy( linked_list_t *lst) {

  if( LLSTgetCount( lst) > 0) {
    fprintf( stderr, 
             " ** Info ** : Destroying non-empty list. [%d elements]\n\n", 
           LLSTgetCount( lst));
  }
  while( LLSTgetCount( lst) != 0) {
    LLSTremoveCurrent( lst);
    LLSTnext( lst);
  }
  SNetMemFree( lst);
}
/* ========================================================================= */

/* true <=> recordLevel identical and record number identical */
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


static void *DetCollector( void *info) {
  int i,j;
  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  sem_t *sem;
  snet_record_t *rec;
  linked_list_t *lst = NULL; 
  bool terminate = false;
  bool got_record;
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

  lst = LLSTcreate( elem);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
    processed_record = false;
    j = LLSTgetCount( lst); /*XXX List operation */
    for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
      tmp_elem = LLSTget( lst); 
      rec = SNetBufShow( tmp_elem->buf);
      if( rec != NULL) { 
       
        got_record = true;
        
        switch( SNetRecGetDescriptor( rec)) {
          case REC_data:
            tmp_elem = LLSTget( lst); 
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              SNetBufPut( outbuf, rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst); 
            }
                
            //rec = SNetBufGet( current_buf);
            //SNetBufPut( outbuf, rec);
            break;
  
          case REC_sync:
            tmp_elem = LLSTget( lst); 
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem->buf = SNetRecGetBuffer( rec);
             LLSTset( lst, tmp_elem); 
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst); 
            }
            break;
  
          case REC_collect:
            tmp_elem = LLSTget( lst); 
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             new_elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
             new_elem->buf = SNetRecGetBuffer( rec);
             if( current_sort_rec == NULL) {
               new_elem->current = NULL;
             }
             else {
               new_elem->current = SNetRecCopy( current_sort_rec);
             }
             LLSTadd( lst, new_elem); 
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst); 
            }            
            break;
          case REC_sort_begin:
            tmp_elem = LLSTget( lst); 
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem->current = SNetRecCopy( rec);
              counter += 1;

              if( counter == LLSTgetCount( lst)) { 
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
              LLSTnext( lst); 
            }
            break;
          case REC_sort_end:
            tmp_elem = LLSTget( lst); 
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              sort_end_counter += 1;

              if( sort_end_counter == LLSTgetCount( lst)) {
                sort_end_counter = 0;

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
              LLSTnext( lst);
            }
            break;
          case REC_terminate:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem = LLSTget( lst);
              SNetBufDestroy( tmp_elem->buf);
              LLSTremoveCurrent( lst);
              SNetMemFree( tmp_elem);
              if( LLSTgetCount( lst) == 0) {
                terminate = true;
                SNetBufPut( outbuf, rec);
                SNetBufBlockUntilEmpty( outbuf);
                SNetBufDestroy( outbuf);
                LLSTdestroy( lst);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }

            break;
        } // switch
      } // if
      else {
        LLSTnext( lst);
      }
//    } // while !got_record
    }
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
  linked_list_t *lst = NULL;
  bool terminate = false;
  bool got_record;
  bool processed_record;
  snet_collect_elem_t *tmp_elem, *elem;

  snet_record_t *current_sort_rec = NULL;
  
  int counter = 0; 
  int sort_end_counter = 0;

  sem = SNetMemAlloc( sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher( inf->from_buf, sem);
  
  elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = LLSTcreate( elem);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
    got_record = false;
    processed_record = false;
    j = LLSTgetCount( lst);
    for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
      current_buf = ((snet_collect_elem_t*) LLSTget( lst))->buf;
      rec = SNetBufShow( current_buf);
      if( rec != NULL) { 
       
        got_record = true;
        
        switch( SNetRecGetDescriptor( rec)) {
          case REC_data:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              SNetBufPut( outbuf, rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
                
            //rec = SNetBufGet( current_buf);
            //SNetBufPut( outbuf, rec);
            break;
  
          case REC_sync:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem = LLSTget( lst);
             tmp_elem->buf = SNetRecGetBuffer( rec);
             LLSTset( lst, tmp_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
  
          case REC_collect:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
             tmp_elem->buf = SNetRecGetBuffer( rec);
             if( current_sort_rec == NULL) {
               tmp_elem->current = NULL;
             }
             else {
               tmp_elem->current = SNetRecCopy( current_sort_rec);
             }
             LLSTadd( lst, tmp_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }            
            break;
          case REC_sort_begin:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem->current = SNetRecCopy( rec);
              counter += 1;

              if( counter == LLSTgetCount( lst)) {
                SNetRecDestroy( current_sort_rec);
                current_sort_rec = SNetRecCopy( rec);
                counter = 0;
                SNetBufPut( outbuf, rec);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_sort_end:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              sort_end_counter += 1;
              if( sort_end_counter == LLSTgetCount( lst)) {
                SNetBufPut( outbuf, rec);
                sort_end_counter = 0;
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_terminate:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem = LLSTget( lst);
              SNetBufDestroy( tmp_elem->buf);
              LLSTremoveCurrent( lst);
              SNetMemFree( tmp_elem);
              if( LLSTgetCount( lst) == 0) {
                terminate = true;
                SNetBufPut( outbuf, rec);
                SNetBufBlockUntilEmpty( outbuf);
                SNetBufDestroy( outbuf);
                LLSTdestroy( lst);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }

            break;
        } // switch
      } // if
      else {
        LLSTnext( lst);
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

