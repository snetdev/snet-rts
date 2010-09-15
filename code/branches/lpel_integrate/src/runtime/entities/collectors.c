#include <stdlib.h>

#include "collectors.h"
#include "snetentities.h"
#include "memfun.h"
#include "handle.h"
#include "record.h"
#include "bool.h"
#include "debug.h"

#include "threading.h"

#include "stream.h"
#include "task.h"
#include "linklst.h"

//#define COLLECTOR_DEBUG



/**
 * @pre rec1 and rec2 are sort records
 */
static bool SortRecEqual( snet_record_t *rec1, snet_record_t *rec2)
{
  return (SNetRecGetLevel(rec1) == SNetRecGetLevel(rec2)) &&
         (SNetRecGetNum(  rec1) == SNetRecGetNum(  rec2)); 
}


/**
 * Collector Task
 */
void CollectorTask( task_t *self, void *arg)
{
  snet_handle_t *hnd = (snet_handle_t *)arg;
  list_hnd_t readyset, waitingset;
  list_iter_t *iter;
  stream_mh_t *outstream;
  snet_record_t *sort_rec = NULL;
  snet_record_t *term_rec = NULL;
  bool terminate = false;

  /* open outstream for writing */
  outstream = StreamOpen( self, SNetHndGetOutput( hnd), 'w');

  readyset = waitingset = NULL;
  {
    stream_t **instreams = SNetHndGetInputs( hnd);
    int num = SNetHndGetNum( hnd);
    int i;
    /* fill initial readyset of collector */
    for (i=0; i<num; i++) {
      stream_mh_t *tmp;
      /* open each stream in listening set for reading */
      tmp = StreamOpen( self, instreams[i], 'r');
      /* add each stream instreams[i] to listening set of collector */
      list_node_t *node = ListNodeCreate( tmp);
      ListAppend( &readyset, node);
    }
    SNetMemFree( instreams );
  }

  /* create an iterator, is reused within main loop*/
  iter = ListIterCreate( &readyset);
  
  /* MAIN LOOP */
  while( !terminate) {

    /* iterate through readyset: for each node (stream) */
    ListIterReset( &readyset, iter);
    while( ListIterHasNext(iter)) {
      list_node_t *cur_node = ListIterNext(iter);
      stream_mh_t *cur_stream = (stream_mh_t *)ListNodeGet( cur_node);
      bool do_next = false;
  
      /* process stream until empty or next sort record or empty */
      while ( !do_next && StreamPeek( cur_stream) != NULL) {
        snet_record_t *rec = StreamRead( cur_stream);

        switch( SNetRecGetDescriptor( rec)) {

          case REC_sort_end:
            /* sort record: place node in waitingset */
            /* remove node */
            ListIterRemove(iter);
            /* put in waiting set */
            ListAppend( &waitingset, cur_node);

            if (sort_rec!=NULL) {
              /* assure that level & counter match */
              if( !SortRecEqual(rec, sort_rec) ) {
                SNetUtilDebugNotice(
                    "[COLL] Warning: Received sort records do not match!");
              }
              /* destroy record */
              SNetRecDestroy( rec);
            } else {
              sort_rec = rec;
            }
            /* end processing this stream */
            do_next = true;
            break;

          case REC_data:
            /* data record: forward to output */
            StreamWrite( outstream, rec);
            break;

          case REC_sync:
            /* sync: replace stream */
            StreamReplace( cur_stream, SNetRecGetStream( rec));
            /* destroy record */
            SNetRecDestroy( rec);
            break;

          case REC_collect:
            /* collect: add new stream to listening set */
            {
              /* new stream */
              stream_mh_t *new_mh = StreamOpen( self,
                  SNetRecGetStream( rec), 'r');
              /* create new node */
              list_node_t *newnode = ListNodeCreate( new_mh);
              /* add to readyset (via iterator: after current) */
              ListIterAppend( iter, newnode);
            }
            /* destroy record */
            SNetRecDestroy( rec);
            break;

          case REC_terminate:
            /* termination record: close stream and remove from ready set */
            if ( !ListIsEmpty( &waitingset)) {
              SNetUtilDebugNotice("[COLL] Warning: Termination record "
                  "received while waiting on sort records!\n");
            }
            StreamClose( cur_stream, true);
            ListIterRemove( iter);
            ListNodeDestroy( cur_node);
            if (term_rec==NULL) {
              term_rec = rec;
            } else {
              /* destroy record */
              SNetRecDestroy( rec);
            }
            /* stop processing this stream */
            do_next = true;
            break;

          default:
            SNetUtilDebugNotice(
                "[COLL] Unknown control record destroyed (%d).\n",
                SNetRecGetDescriptor( rec));
            SNetRecDestroy( rec);
        } /* end switch */
      } /* end current stream*/
    } /* end for each stream in ready */

    /* one iteration finished through readyset */
    
    /* check if all sort records are received */
    if ( ListIsEmpty( &readyset)) {
      
      if ( !ListIsEmpty( &waitingset)) {
        /* "swap" sets */
        readyset = waitingset;
        waitingset = NULL;
        /* check sort record: if level > 0,
           decrement and forward to outstream */
        assert( sort_rec != NULL);
        {
          int level = SNetRecGetLevel( sort_rec);
          if( level > 0) {
            SNetRecSetLevel( sort_rec, level-1);
            StreamWrite( outstream, sort_rec);
          } else {
            SNetRecDestroy( sort_rec);
          }
          sort_rec = NULL;
        }
      } else {
        /* send a terminate record */
        StreamWrite( outstream, term_rec);
        /* waitingset is also empty: terminate*/
        terminate = true;
      }

    } else {
      /* readyset is not empty: continue "collecting" sort records */
      /*TODO rotate readyset? */
      /* wait on new input */
      TaskWaitOnAny( self);
    }

  } /* MAIN LOOP END*/

  /* close outstream */
  StreamClose( outstream, false);

  /* destroy iterator */
  ListIterDestroy( iter);

  SNetHndDestroy( hnd);

} /* Collector task end*/




/**
 * Collector creation function
 * @pre num >= 1
 */
stream_t *CollectorCreate( int num, stream_t **instreams)
{
  stream_t *outstream;
  snet_handle_t *hnd;

  /* create outstream */
  outstream = StreamCreate();

  /* create collector handle */
  hnd = SNetHndCreate( HND_collector, instreams, outstream, num);
  /* spawn collector task */
  SNetEntitySpawn( CollectorTask, (void *)hnd, ENTITY_collect);
  return outstream;
}

