#include <stdlib.h>
#include <assert.h>

#include "collector.h"
#include "snetentities.h"
#include "locvec.h"
#include "memfun.h"
#include "bool.h"
#include "debug.h"

#include "threading.h"


//#define DEBUG_PRINT_GC


/*
 * Streams that are in the waitingset are not inspected until
 * the waitingset becomes the readyset again.
 * It is possible that the next record on a stream in the waitingset
 * is a termination record, which means that the stream is not required
 * anymore and could be destroyed.
 *
 * Following flag causes a scanning of the streams in the waiting set
 * upon arrival of a collect record to find and destroy such streams that
 * are not required anymore.
 */
#define DESTROY_TERM_IN_WAITING_UPON_COLLECT

typedef struct {
  snet_stream_t *output;
  void* inputs; /* either snet_stream_t* or snet_stream_t** */
  int num;
  snet_locvec_t *myloc;
  bool dynamic;
} coll_arg_t;


//FIXME in threading layer?
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
 * Peeks the top record of all streams in waitingset,
 * if there is a terminate record, the stream is closed
 * and removed from the waitingset
 */
static int DestroyTermInWaitingSet(snet_stream_iter_t *wait_iter,
    snet_streamset_t *waitingset)
{
  int destroy_cnt=0;
  if ( !SNetStreamsetIsEmpty( waitingset)) {
    SNetStreamIterReset( wait_iter, waitingset);
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
        /* update destroyed counter */
        destroy_cnt++;
        /* destroy record */
        SNetRecDestroy( wait_rec);
      }
      /* else do nothing */
    }
  }
  return destroy_cnt;
}



/**
 * Collector Task
 */
void CollectorTask(void *arg)
{
  coll_arg_t *carg = (coll_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *wait_iter;
  snet_stream_desc_t *outstream;
  snet_record_t *sort_rec = NULL;
  snet_record_t *term_rec = NULL;
  snet_record_t *source_rec = NULL;
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
      tmp = SNetStreamOpen( ((snet_stream_t **)carg->inputs)[i], 'r');
      /* add each stream instreams[i] to listening set of collector */
      SNetStreamsetPut( &readyset, tmp);
    }
    SNetMemFree( carg->inputs );
  }

  /* create an iterator for waiting set, is reused within main loop*/
  wait_iter = SNetStreamIterCreate( &waitingset);

  /* MAIN LOOP */
  while( !terminate) {

    /*XXX improve? is_dynamic && |readyset|==1 => StreamRead(initial) */
    snet_stream_desc_t *cur_stream = SNetStreamPoll(&readyset);
    bool do_next = false;


    /* process stream until empty or next sort record or empty */
    while ( !do_next && SNetStreamPeek( cur_stream) != NULL) {
      snet_record_t *rec = SNetStreamRead( cur_stream);
      int res = -1;

      switch( SNetRecGetDescriptor( rec)) {

        case REC_sort_end:
          /* sort record: place node in waitingset */
          /* remove node */
          res = SNetStreamsetRemove( &readyset, cur_stream);
          assert(res == 0);
          /* put in waiting set */
          SNetStreamsetPut( &waitingset, cur_stream);

          if (sort_rec!=NULL) {
            /*
             * check that level & counter match
             */
            if( !SortRecEqual(rec, sort_rec) ) {
              SNetUtilDebugNoticeLoc( carg->myloc,
                  "[COLL] Warning: Received sort records do not match! "
                  "expected (l%d,c%d) got (l%d,c%d)", /*TROLLFACE*/
                  SNetRecGetLevel(sort_rec), SNetRecGetNum(sort_rec),
                  SNetRecGetLevel(rec),      SNetRecGetNum(rec)
                  );
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
#ifdef DEBUG_PRINT_GC
          //FIXME FIXME FIXME
          if (!carg->dynamic) {
            SNetUtilDebugNoticeLoc(carg->myloc, "[COLL] received REC_sync on %p!", cur_stream);
          }
#endif
          /* sync: replace stream */
          SNetStreamReplace( cur_stream, SNetRecGetStream( rec));
          /* destroy record */
          SNetRecDestroy( rec);
          /*
           * *** !!! IMPORTANT FOR GC !!! ***
           *
           * Leave the loop to check if we can terminate.
           * Source and according sync record have to travel in pairs.
           * Any subsequent sync record on this stream must arrive at the successor entity.
           * TODO reorder, put in REC_source handler
           */
          if (source_rec != NULL) {
            do_next = true;
          }
          break;

        case REC_collect:
          assert( carg->dynamic == true );
          /* collect: add new stream to ready set */
#ifdef DESTROY_TERM_IN_WAITING_UPON_COLLECT
          /* but first, check if we can free resources by checking the
             waiting set for arrival of termination records */
          incount -= DestroyTermInWaitingSet(wait_iter, &waitingset);
          assert(incount > 0);
#endif
          /* finally, add new stream to ready set */
          {
            /* new stream */
            snet_stream_desc_t *new_sd = SNetStreamOpen(
                SNetRecGetStream( rec), 'r');
            /* add to readyset */
            SNetStreamsetPut( &readyset, new_sd);
            /* update incoming counter */
            incount++;
          }
          /* destroy collect record */
          SNetRecDestroy( rec);
          break;

        case REC_terminate:
          /* termination record: close stream and remove from ready set */

          /* Pre garbage-collection it was impossible that a termination
           * record was received while waiting on sort records.
           */
          //assert( !SNetStreamsetIsEmpty( &waitingset) );
#ifdef DEBUG_PRINT_GC
          //FIXME FIXME FIXME
          if (!carg->dynamic) {
            SNetUtilDebugNoticeLoc(carg->myloc, "[COLL] received REC_terminate on %p!", cur_stream);
          }
#endif
          res = SNetStreamsetRemove( &readyset, cur_stream);
          assert(res == 0);
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


        case REC_source:
          /**
           * Invariant: Collector will ever receive only one source record
           */
          assert(source_rec == NULL);
          source_rec = rec;
          assert(1 == incount);
#ifdef DEBUG_PRINT_GC
          //FIXME FIXME FIXME
          if (!carg->dynamic) {
            SNetUtilDebugNoticeLoc(carg->myloc, "[COLL] received REC_source on %p!", cur_stream);
          }
#endif
          break;

        default:
          assert(0);
          /* if ignore, at least destroy ... */
          SNetRecDestroy( rec);
      } /* end switch */
    } /* end current stream*/



    /* state update */
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
#ifdef DEBUG_PRINT_GC
        if (!carg->dynamic) {
          SNetUtilDebugNoticeLoc( carg->myloc,
              "[COLL] Terminate static collector as no inputs left!"
              );
        }
#endif
      }

    } else if (!carg->dynamic && incount==1) {
      /* static collector has only one incoming stream left
       * -> this input stream can be sent to the collectors
       *    successor via a sync record
       */
      snet_stream_desc_t *in = readyset;
      assert( SNetStreamsetIsEmpty(&waitingset) );

      if (source_rec != NULL) {
        SNetStreamWrite(outstream, source_rec);
        source_rec = NULL;
      };

      SNetStreamWrite( outstream,
          SNetRecCreate( REC_sync, SNetStreamGet(in))
          );
      SNetStreamClose( in, false);
      terminate = true;
#ifdef DEBUG_PRINT_GC
      /* terminating due to GC */
      SNetUtilDebugNoticeLoc( carg->myloc,
          "[COLL] Terminate static collector as only one branch left!"
          );
#endif
    }

  } /* MAIN LOOP END*/


  if (source_rec != NULL) {
    SNetRecDestroy(source_rec);
  }

  /* close outstream */
  SNetStreamClose( outstream, false);

  /* destroy iterator */
  SNetStreamIterDestroy( wait_iter);

  SNetLocvecDestroy(carg->myloc);

  SNetMemFree( carg);

} /* Collector task end*/




/**
 * Collector creation function
 * @pre num >= 1
 */
snet_stream_t *CollectorCreateStatic( int num, snet_stream_t **instreams, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;
  int i;

  /* create outstream */
  outstream = (snet_stream_t*) SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  /* copy instreams */
  carg->inputs = SNetMemAlloc(num * sizeof(snet_stream_t *));
  for(i=0; i<num; i++) {
    ((snet_stream_t **)carg->inputs)[i] = instreams[i];
  }
  carg->output = outstream;
  carg->num = num;
  carg->myloc = SNetLocvecCopy(SNetLocvecGet(info));
  carg->dynamic = false;

  /* spawn collector task */
  SNetEntitySpawn( ENTITY_collect, SNetLocvecGet(info), location,
      "<collect>", CollectorTask, (void*)carg);
  return outstream;
}



/**
 * Collector creation function
 */
snet_stream_t *CollectorCreateDynamic( snet_stream_t *instream, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_arg_t *carg;

  /* create outstream */
  outstream = (snet_stream_t*) SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->inputs = instream;
  carg->output = outstream;
  carg->num = 1;
  carg->myloc = SNetLocvecCopy(SNetLocvecGet(info));
  carg->dynamic = true;

  /* spawn collector task */
  SNetEntitySpawn( ENTITY_collect, SNetLocvecGet(info), location,
      "<collect>", CollectorTask, (void*)carg);
  return outstream;
}

