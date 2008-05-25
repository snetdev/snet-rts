#include "split.h"
#include "stdlib.h" // for list
#include "stdio.h" // for list

#include "memfun.h"
#include "buffer.h"
#include "handle.h"
#include "debug.h"
#include "collectors.h"
#include "threading.h"

/* hkr: TODO: remove this list, replace with SNetUtilList */

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
/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */




snet_blist_elem_t *FindBufInList( linked_list_t *l, int num) {

  int i;
  snet_blist_elem_t *elem;
  for( i=0; i < LLSTgetCount( l); i++) {
    elem = (snet_blist_elem_t*)LLSTget( l);
    if( elem->num == num) {
      return( elem);
    } 
    else {
      LLSTnext( l);
    }
  }
  return( NULL);
}

static void *SplitBoxThread( void *hndl) {

  int i;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t *initial;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec, *current_sort_rec;
  snet_buffer_t* (*boxfun)( snet_buffer_t*);
  bool terminate = false;
  linked_list_t *repos = NULL;
  snet_blist_elem_t *elem;


  initial = SNetHndGetOutbuffer( hnd);
  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);

  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        
        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->buf = SNetBufCreate( BUFFER_SIZE);
          SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
          //SNetBufBlockUntilEmpty( initial);
          repos = LLSTcreate( elem);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList( repos, i);
          if( elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->buf = SNetBufCreate( BUFFER_SIZE);
            LLSTadd( repos, elem);
            SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
            //SNetBufBlockUntilEmpty( initial);
          } 
          if( i == utag_val) {
            SNetBufPut( elem->buf, rec); // last rec is not copied.
          }
          else {
            SNetBufPut( elem->buf, SNetRecCopy( rec)); // COPY
          }
        } 
        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);

        break;
      case REC_collect:
        SNetUtilDebugNotice("[SPLIT] Unhandled control record," 
                            " destroying it \n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        current_sort_rec = rec;
        for( i=0; i<( LLSTgetCount( repos) - 1); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, 
              SNetRecCreate( REC_sort_begin, SNetRecGetLevel( rec), 
                             SNetRecGetNum( rec)));
          LLSTnext( repos);
        }
        elem = LLSTget( repos);
        SNetBufPut( elem->buf, rec);
        break;
      case REC_sort_end:
        current_sort_rec = rec;
        for( i=0; i<( LLSTgetCount( repos) - 1); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, 
              SNetRecCreate( REC_sort_end, SNetRecGetLevel( rec), 
                             SNetRecGetNum( rec)));
          LLSTnext( repos);
        }
        elem = LLSTget( repos);
        SNetBufPut( elem->buf, rec);      
        break;
      case REC_terminate:
        terminate = true;

        for( i=0; i<LLSTgetCount( repos); i++) {
          elem = LLSTget( repos);
          SNetBufPut( (snet_buffer_t*)elem->buf, SNetRecCopy( rec));
          LLSTnext( repos);
        }
        while( LLSTgetCount( repos) > 0) {
          LLSTremoveCurrent( repos);
          LLSTnext( repos);
        }
        LLSTdestroy( repos);

        SNetBufPut( initial, rec);
        break;
    }
  }

  return( NULL);
}



extern snet_buffer_t *SNetSplit( snet_buffer_t *inbuf, 
                                 snet_buffer_t* (*box_a)( snet_buffer_t*),
                                 int ltag, int utag) {

  snet_buffer_t *initial, *outbuf;
  snet_handle_t *hnd;

  initial = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, inbuf, initial, box_a, ltag, utag);
 
  outbuf = CreateCollector( initial);

  SNetThreadCreate( SplitBoxThread, (void*)hnd, ENTITY_split_nondet);

  return( outbuf);
}



static void *DetSplitBoxThread( void *hndl) {

  int i,j;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t *initial, *tmp;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec;
  snet_buffer_t* (*boxfun)( snet_buffer_t*);
  bool terminate = false;
  linked_list_t *repos = NULL;
  snet_blist_elem_t *elem;

  int counter = 1;


  initial = SNetHndGetOutbuffer( hnd);
  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);

  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        
        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->buf = SNetBufCreate( BUFFER_SIZE);
          SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, 0));
          
          tmp = boxfun( elem->buf);
          SNetBufPut( initial, SNetRecCreate( REC_collect, tmp));
//          SNetBufPut( tmp, SNetRecCreate( REC_sort_begin, 0, 0));
          SNetBufPut( tmp, SNetRecCreate( REC_sort_end, 0, 0));

          SNetBufPut( initial, SNetRecCreate( REC_sort_end, 0, 0));
          repos = LLSTcreate( elem);
        }


        SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, counter)); 
        for( j=0; j<LLSTgetCount( repos); j++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter)); 
          LLSTnext( repos);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList( repos, i);
          if( elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->buf = SNetBufCreate( BUFFER_SIZE);
            LLSTadd( repos, elem);
            SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
//            SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter));
          }
          if( i == utag_val) {
            SNetBufPut( elem->buf, rec); // last rec is not copied.
          }
          else {
            SNetBufPut( elem->buf, SNetRecCopy( rec)); // COPY
          }
        }

        for( j=0; j<LLSTgetCount( repos); j++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_end, 0, counter)); 
          LLSTnext( repos);
        }
        SNetBufPut( initial, SNetRecCreate( REC_sort_end, 0, counter)); 
        counter += 1;

        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        SNetUtilDebugNotice("[SPLIT] Unhandled control record," 
                            " destroying it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;

        for( i=0; i<LLSTgetCount( repos); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter)); 
          SNetBufPut( (snet_buffer_t*)elem->buf, SNetRecCopy( rec));
          LLSTnext( repos);
        }
        while( LLSTgetCount( repos) > 0) {
          LLSTremoveCurrent( repos);
          LLSTnext( repos);
        }
        LLSTdestroy( repos);

        SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, counter)); 
        SNetBufPut( initial, rec);
        break;
    }
  }

  return( NULL);
}



extern
snet_buffer_t *SNetSplitDet( snet_buffer_t *inbuf, 
                             snet_buffer_t *(*box_a)( snet_buffer_t*),
                                       int ltag, 
                                       int utag) {

  snet_buffer_t *initial, *outbuf;
  snet_handle_t *hnd;

  initial = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, inbuf, initial, box_a, ltag, utag);
 
  outbuf = CreateDetCollector( initial);

  SNetThreadCreate( DetSplitBoxThread, (void*)hnd, ENTITY_split_det);

  return( outbuf);
}

