#include <stdlib.h>

#include "collectors.h"
#include "snetentities.h"
#include "memfun.h"
#include "handle.h"
#include "record.h"
#include "bool.h"
#include "debug.h"

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
  stream_t *outstream;
  snet_record_t *sort_rec = NULL;
  bool terminate = false;

  /* open outstream for writing */
  outstream = SNetHndGetOutput( hnd);
  StreamOpen( self, outstream, 'w');

  readyset = waitingset = NULL;
  {
    stream_t **instreams = SNetHndGetInputs( hnd);
    int num = SNetHndGetNum( hnd);
    int i;
    /* fill initial readyset of collector */
    for (i=0; i<num; i++) {
      /* open each stream in listening set for reading */
      StreamOpen( self, instreams[i], 'r');
      /* add each stream instreams[i] to listening set of collector */
      list_node_t *node = ListNodeCreate( instreams[i]);
      ListAppend( &readyset, node);
    }
    if (num>1) SNetMemFree( instreams );
  }

  /* create an iterator, is reused within main loop*/
  iter = ListIterCreate( &readyset);
  
  /* MAIN LOOP */
  while( !terminate) {

    /* iterate through readyset: for each node (stream) */
    ListIterReset( &readyset, iter);
    while( ListIterHasNext(iter)) {
      list_node_t *cur_node = ListIterNext(iter);
      stream_t *cur_stream = (stream_t *)ListNodeGet( cur_node);
      bool do_next = false;
  
      /* process stream until empty or next sort record or empty */
      while ( !do_next && StreamPeek( self, cur_stream) != NULL) {
        snet_record_t *rec = StreamRead( self, cur_stream);

        switch( SNetRecGetDescriptor( rec)) {

          case REC_sort_begin:
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
            StreamWrite( self, outstream, rec);
            break;

          case REC_sync:
            /* sync: replace stream, also in listening set */
            StreamReplace( self, &cur_stream, SNetRecGetStream( rec));
            ListNodeSet( cur_node, cur_stream);
            /* destroy record */
            SNetRecDestroy( rec);
            break;

          case REC_collect:
            /* collect: add new stream to listening set */
            {
              /* create new node */
              list_node_t *newnode = ListNodeCreate( SNetRecGetStream( rec));
              StreamOpen( self, ListNodeGet(newnode), 'r');
              /* add to readyset (valid during iteration!) */
              ListAppend( &readyset, newnode);
            }
            /* destroy record */
            SNetRecDestroy( rec);
            break;

          case REC_terminate:
            /* termination record: close stream and remove from ready set */
            if ( !ListIsEmpty( &waitingset)) {
              SNetUtilDebugNotice("[COLL] Warning: Termination record"
                  "received while waiting on sort records!\n");
            }
            StreamClose( self, cur_stream);
            StreamDestroy( cur_stream);
            ListIterRemove( iter);
            ListNodeDestroy( cur_node);
            /* destroy record */
            SNetRecDestroy( rec);
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
        /* check sort record: if level > 0, forward to outstream */
        if( SNetRecGetLevel( sort_rec) > 0) {
          StreamWrite( self, outstream, sort_rec);
        } else {
          SNetRecDestroy( sort_rec);
          sort_rec = NULL;
        }
      } else {
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
  StreamClose( self, outstream);
  StreamDestroy( outstream);

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

