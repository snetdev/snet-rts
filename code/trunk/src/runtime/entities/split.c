#include <stdlib.h>

#include "split.h"

#include "memfun.h"
#include "buffer.h"
#include "handle.h"
#include "debug.h"
#include "collectors.h"
#include "threading.h"
#include "stream_layer.h"
#include "list.h"
/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */




snet_blist_elem_t *FindBufInList(snet_util_list_t *list, int num) {
  snet_blist_elem_t *elem;
  snet_util_list_iter_t *current_position;

  current_position = SNetUtilListFirst(list);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    elem = (snet_blist_elem_t*)SNetUtilListIterGet(current_position);

    if( elem->num == num) {
      return elem;
    }
    else {
      current_position = SNetUtilListIterNext(current_position);
    }
  }
  return(NULL);
}

static void *SplitBoxThread( void *hndl) {
  snet_util_list_iter_t *current_position;

  int i;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_tl_stream_t *initial;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec, *current_sort_rec;
  snet_tl_stream_t* (*boxfun)( snet_tl_stream_t*);
  bool terminate = false;
  snet_util_list_t *repos = NULL;
  snet_blist_elem_t *elem;


  initial = SNetHndGetOutput( hnd);
  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);

  
  while( !( terminate)) {
    rec = SNetTlRead( SNetHndGetInput( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        SNetUtilDebugNotice("SPLIT got data");
        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->stream = SNetTlCreateStream( BUFFER_SIZE);
          SNetTlWrite( initial, 
                       SNetRecCreate( REC_collect, boxfun( elem->stream)));
          //SNetBufBlockUntilEmpty( initial);
          repos = SNetUtilListCreate();
          repos = SNetUtilListAddBeginning(repos, elem);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList( repos, i);
          if( elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->stream = SNetTlCreateStream( BUFFER_SIZE);
            repos = SNetUtilListAddBeginning(repos, elem);
            SNetTlWrite( initial, 
                         SNetRecCreate( REC_collect, boxfun( elem->stream)));
            //SNetBufBlockUntilEmpty( initial);
          } 
          if( i == utag_val) {
            SNetTlWrite( elem->stream, rec); // last rec is not copied.
          }
          else {
            SNetTlWrite( elem->stream, SNetRecCopy( rec)); // COPY
          }
        } 
        break;
      case REC_sync:
        SNetHndSetInput( hnd, SNetRecGetStream( rec));
        SNetRecDestroy( rec);

        break;
      case REC_collect:
        SNetUtilDebugNotice("[SPLIT] Unhandled control record,"
                            " destroying it \n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        current_sort_rec = rec;
        if(!SNetUtilListIsEmpty(repos)) {
          // elven queen
          current_position = SNetUtilListFirst(repos);
          while(SNetUtilListIterHasNext(current_position)) {
            elem = SNetUtilListIterGet(current_position);
            SNetTlWrite( elem->stream,
                SNetRecCreate( REC_sort_begin, SNetRecGetLevel( rec),
                               SNetRecGetNum( rec)));
            current_position = SNetUtilListIterNext(current_position);
          }
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, rec);
          SNetUtilListIterDestroy(current_position);
        } else {
          /* we are unable to relay this record anyway, so we discard it? */
          SNetUtilDebugNotice("[SPLIT] Got sort_begin_record with nowhere to"
                     " send it!");
          SNetRecDestroy(rec);
        }
        break;
      case REC_sort_end:
        current_sort_rec = rec;
        if(!SNetUtilListIsEmpty(repos)) {
          current_position = SNetUtilListFirst(repos);
          while(SNetUtilListIterHasNext(current_position)) {
            elem = SNetUtilListIterGet(current_position);
            SNetTlWrite( elem->stream,
                SNetRecCreate( REC_sort_end, SNetRecGetLevel( rec),
                               SNetRecGetNum( rec)));
            current_position = SNetUtilListIterNext(current_position);
          }
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, rec);
          SNetUtilListIterDestroy(current_position);
        } else {
          SNetUtilDebugNotice("[SPLIT] Got sort_end_record with nowhere to"
                      " send it!");
          SNetRecDestroy(rec);
        }
        break;
      case REC_terminate:
        terminate = true;

        if(!SNetUtilListIsEmpty(repos)) {
          current_position = SNetUtilListFirst(repos);
          while(SNetUtilListIterHasNext(current_position)) {
            elem = SNetUtilListIterGet(current_position);
            SNetTlWrite(elem->stream, SNetRecCopy( rec));
            current_position = SNetUtilListIterNext(current_position);
          }
          SNetUtilListIterDestroy(current_position);
        } else {
          SNetUtilDebugNotice("[SPLIT] got termination record with nowhere"
              " send it!");
          SNetRecDestroy(rec);
        }
        SNetUtilListDestroy(repos);

        SNetTlWrite(initial, rec);
        break;
      case REC_probe:
        current_position = SNetUtilListFirst(repos);
        elem = SNetUtilListIterGet(current_position);
        SNetTlWrite(elem->stream, rec);
        current_position = SNetUtilListIterNext(current_position);
        while(SNetUtilListIterCurrentDefined(current_position)) {
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, SNetRecCopy(rec));
          current_position = SNetUtilListIterNext(current_position);
        }
      break;
    }
  }

  return( NULL);
}



extern snet_tl_stream_t *SNetSplit( snet_tl_stream_t *input, 
                                snet_tl_stream_t* (*box_a)( snet_tl_stream_t*),
                                int ltag, int utag) {

  snet_tl_stream_t *initial, *output;
  snet_handle_t *hnd;

  initial = SNetTlCreateStream( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, input, initial, box_a, ltag, utag);
 
  output = CreateCollector(initial);

  SNetTlCreateComponent(SplitBoxThread, (void*)hnd, ENTITY_split_nondet);
  return(output);
}



static void *DetSplitBoxThread( void *hndl) {
  int i;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_tl_stream_t *initial, *tmp;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec;
  snet_tl_stream_t* (*boxfun)( snet_tl_stream_t*);
  bool terminate = false;
  snet_util_list_t *repos = NULL;
  snet_blist_elem_t *elem;
  snet_util_list_iter_t *current_position;
  int counter = 1;


  initial = SNetHndGetOutput(hnd);
  boxfun = SNetHndGetBoxfun(hnd);
  ltag = SNetHndGetTagA(hnd);
  utag = SNetHndGetTagB(hnd);


  while( !( terminate)) {
    rec = SNetTlRead(SNetHndGetInput( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);

        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->stream = SNetTlCreateStream(BUFFER_SIZE);
          SNetTlWrite(initial, SNetRecCreate(REC_sort_begin, 0, 0));

          tmp = boxfun(elem->stream);
          SNetTlWrite(initial, SNetRecCreate(REC_collect, tmp));
//          SNetBufPut(tmp, SNetRecCreate( REC_sort_begin, 0, 0));
          SNetTlWrite(tmp, SNetRecCreate(REC_sort_end, 0, 0));

          SNetTlWrite(initial, SNetRecCreate(REC_sort_end, 0, 0));
          repos = SNetUtilListCreate();
          repos = SNetUtilListAddBeginning(repos, elem);
        }


        SNetTlWrite(initial, SNetRecCreate( REC_sort_begin, 0, counter));
        current_position = SNetUtilListFirst(repos);
        while(SNetUtilListIterCurrentDefined(current_position)) {
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, 
                      SNetRecCreate( REC_sort_begin, 0, counter));
          current_position = SNetUtilListIterNext(current_position);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList(repos, i);
          if(elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->stream = SNetTlCreateStream(BUFFER_SIZE);
            repos = SNetUtilListAddBeginning(repos, elem);
            SNetTlWrite( initial, 
                         SNetRecCreate( REC_collect, boxfun( elem->stream)));
          }
          if( i == utag_val) {
            SNetTlWrite(elem->stream, rec); // last rec is not copied.
          }
          else {
            SNetTlWrite(elem->stream, SNetRecCopy(rec)); // COPY
          }
        }

        current_position = SNetUtilListFirst(repos);
        while(SNetUtilListIterCurrentDefined(current_position)) {
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, SNetRecCreate( REC_sort_end, 0, counter));
          current_position = SNetUtilListIterNext(current_position);
        }
        SNetTlWrite(initial, SNetRecCreate( REC_sort_end, 0, counter));
        counter += 1;

        break;
      case REC_sync:
        SNetHndSetInput( hnd, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        SNetUtilDebugNotice("[SPLIT] Unhandled control record," 
                            " destroying it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetTlWrite(SNetHndGetOutput(hnd), rec);
        break;
      case REC_sort_end:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetTlWrite(SNetHndGetOutput( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;
        current_position = SNetUtilListFirst(repos);
        while(SNetUtilListIterCurrentDefined(current_position)) {
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, 
                      SNetRecCreate( REC_sort_begin, 0, counter));
          SNetTlWrite(elem->stream, SNetRecCopy(rec));
          current_position = SNetUtilListIterNext(current_position);
        }
        SNetUtilListIterDestroy(current_position);
        SNetUtilListDestroy(repos);

        SNetTlWrite(initial, SNetRecCreate( REC_sort_begin, 0, counter));
        SNetTlWrite(initial, rec);
        break;
      case REC_probe:
        current_position = SNetUtilListFirst(repos);
        elem = SNetUtilListIterGet(current_position);
        SNetTlWrite(elem->stream, SNetRecCreate(REC_sort_begin, 0, counter));
        SNetTlWrite(elem->stream, rec);
        while(SNetUtilListIterCurrentDefined(current_position)) {
          elem = SNetUtilListIterGet(current_position);
          SNetTlWrite(elem->stream, SNetRecCreate(REC_sort_begin, 0, counter));
          SNetTlWrite(elem->stream, SNetRecCopy(rec));
          current_position = SNetUtilListIterNext(current_position);
        }
    }
  }

  return( NULL);
}



extern
snet_tl_stream_t *SNetSplitDet( snet_tl_stream_t *input, 
                             snet_tl_stream_t *(*box_a)( snet_tl_stream_t*),
                                       int ltag, 
                                       int utag) {

  snet_tl_stream_t *initial, *output;
  snet_handle_t *hnd;

  initial = SNetTlCreateStream( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, input, initial, box_a, ltag, utag);
 
  output = CreateDetCollector( initial);

  SNetThreadCreate( DetSplitBoxThread, (void*)hnd, ENTITY_split_det);

  return( output);
}

