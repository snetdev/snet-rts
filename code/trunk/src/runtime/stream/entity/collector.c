#include <stdlib.h>
#include <assert.h>

#include "collector.h"
#include "snetentities.h"
#include "memfun.h"
#include "bool.h"
#include "debug.h"

#include "threading.h"


typedef struct {
  snet_stream_t *output;
  snet_stream_t **inputs;
  int num;
  bool dynamic;
} coll_arg_t;



#define SNetStreamsetIsEmpty(set) ((*set)==NULL)



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
void CollectorTask(void *arg)
{
  coll_arg_t *carg = (coll_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *iter, *wait_iter;
  snet_stream_desc_t *outstream;
  snet_record_t *sort_rec = NULL;
  snet_record_t *term_rec = NULL;
  int incount;
  bool terminate = false;

  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');

  /* initialize input stream counter*/
  incount = carg->num;

  readyset = waitingset = NULL;

  if (carg->dynamic) {
    /* Open initial stream and put into readyset */
    SNetStreamsetPut( &readyset,
        SNetStreamOpen((snet_stream_t *)carg->inputs, 'r')
        );
  } else {
    int i;
    /* fill initial readyset of collector */
    for (i=0; i<incount; i++) {
      snet_stream_desc_t *tmp;
      /* open each stream in listening set for reading */
      tmp = SNetStreamOpen(carg->inputs[i], 'r');
      /* add each stream instreams[i] to listening set of collector */
      SNetStreamsetPut( &readyset, tmp);
    }
    SNetMemFree( carg->inputs );
  }

  /* create an iterator for both sets, is reused within main loop*/
  iter = SNetStreamIterCreate( &readyset);
  wait_iter = SNetStreamIterCreate( &waitingset);

  /* MAIN LOOP */
  while( !terminate) {

    /* iterate through readyset: for each node (stream) */
    SNetStreamIterReset( iter, &readyset);
    while( SNetStreamIterHasNext(iter)) {
      snet_stream_desc_t *cur_stream = SNetStreamIterNext(iter);
      bool do_next = false;

      /* process stream until empty or next sort record or empty */
      while ( !do_next && SNetStreamPeek( cur_stream) != NULL) {
        snet_record_t *rec = SNetStreamRead( cur_stream);

        switch( SNetRecGetDescriptor( rec)) {

          case REC_sort_end:
            /* sort record: place node in waitingset */
            /* remove node */
            SNetStreamIterRemove(iter);
            /* put in waiting set */
            SNetStreamsetPut( &waitingset, cur_stream);

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
            SNetStreamWrite( outstream, rec);
            break;

          case REC_sync:
            /* sync: replace stream */
            SNetStreamReplace( cur_stream, SNetRecGetStream( rec));
            /* destroy record */
            SNetRecDestroy( rec);
            break;

          case REC_collect:
            assert( carg->dynamic == true );
            /* collect: add new stream to ready set */
#if 1
            /* but first, check if we can free resources by checking the
               waiting set for arrival of termination records */
            if ( !SNetStreamsetIsEmpty( &waitingset)) {
              SNetStreamIterReset( wait_iter, &waitingset);
              while( SNetStreamIterHasNext( wait_iter)) {
                snet_stream_desc_t *sd = SNetStreamIterNext( wait_iter);
                snet_record_t *wait_rec = SNetStreamPeek( sd);

                /* for this stream, check if there is a termination record next */
                if ( wait_rec != NULL &&
                    SNetRecGetDescriptor( wait_rec) == REC_terminate ) {
                  /* consume, remove from waiting set and free the stream */
                  (void) SNetStreamRead( sd);
                  SNetStreamIterRemove( wait_iter);
                  SNetStreamClose( sd, true);
                  /* update incoming counter */
                  incount--;
                  assert(incount > 1);
                  /* destroy record */
                  SNetRecDestroy( wait_rec);
                }
                /* else do nothing */
              }
            }
#endif
            /* finally, add new stream to ready set */
            {
              /* new stream */
              snet_stream_desc_t *new_sd = SNetStreamOpen(
                  SNetRecGetStream( rec), 'r');
              /* add to readyset (via iterator: after current) */
              SNetStreamIterAppend( iter, new_sd);
              /* update incoming counter */
              incount++;
            }
            /* destroy collect record */
            SNetRecDestroy( rec);
            break;

          case REC_terminate:
            /* termination record: close stream and remove from ready set */
            if ( !SNetStreamsetIsEmpty( &waitingset)) {
              SNetUtilDebugNotice("[COLL] Warning: Termination record "
                  "received while waiting on sort records!\n");
            }
            SNetStreamIterRemove( iter);
            SNetStreamClose( cur_stream, true);
            /* update incoming counter */
            incount--;
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
            assert(0);
            /* if ignore, at least destroy ... */
            SNetRecDestroy( rec);
        } /* end switch */
      } /* end current stream*/
    } /* end for each stream in ready */

    /* one iteration finished through readyset */

    /* check if all sort records are received */
    if ( SNetStreamsetIsEmpty( &readyset)) {

      if ( !SNetStreamsetIsEmpty( &waitingset)) {
        /*
         * waiting set contains all streams,
         * ready set is empty
         * -> "swap" sets
         */
        readyset = waitingset;
        waitingset = NULL;
        /* check sort record: if level > 0,
           decrement and forward to outstream */
        assert( sort_rec != NULL);
        {
          int level = SNetRecGetLevel( sort_rec);
          if( level > 0) {
            SNetRecSetLevel( sort_rec, level-1);
            SNetStreamWrite( outstream, sort_rec);
          } else {
            SNetRecDestroy( sort_rec);
          }
          sort_rec = NULL;
        }
      } else {
        /* send a terminate record */
        SNetStreamWrite( outstream, term_rec);
        /* waitingset is also empty: terminate*/
        terminate = true;
      }

    } else if (!carg->dynamic && incount==1) {
      /* static collector has only one incoming stream left
       * -> this input stream can be sent to the collectors
       *    successor via a sync record
       */
      snet_stream_desc_t *in = readyset;
      assert( SNetStreamsetIsEmpty(&waitingset) );

      SNetStreamWrite( outstream,
          SNetRecCreate( REC_sync, SNetStreamGet(in))
          );
      SNetStreamClose( in, false);
      terminate = true;
    } else {
      /* readyset is not empty: continue "collecting" sort records */
      /* wait on new input */
      (void) SNetStreamPoll( &readyset);
    }

  } /* MAIN LOOP END*/

  /* close outstream */
  SNetStreamClose( outstream, false);

  /* destroy iterator */
  SNetStreamIterDestroy( iter);

  SNetMemFree( carg);

} /* Collector task end*/




/**
 * Collector creation function
 * @pre num >= 1
 */
snet_stream_t *CollectorCreateStatic( int num, snet_stream_t **instreams, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;

  /* create outstream */
  outstream = (snet_stream_t*) SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->inputs = instreams;
  carg->output = outstream;
  carg->num = num;
  carg->dynamic = false;

  /* spawn collector task */
  SNetEntitySpawn( ENTITY_COLLECT, CollectorTask, (void *)carg);
  return outstream;
}



/**
 * Collector creation function
 */
snet_stream_t *CollectorCreateDynamic( snet_stream_t *instream, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;

  /* create outstream */
  outstream = (snet_stream_t*) SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->inputs = (snet_stream_t **) instream;
  carg->output = outstream;
  carg->num = 1;
  carg->dynamic = true;

  /* spawn collector task */
  SNetEntitySpawn( ENTITY_COLLECT, CollectorTask, (void *)carg);
  return outstream;
}

