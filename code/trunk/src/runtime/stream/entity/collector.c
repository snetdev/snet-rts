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
  snet_stream_t *input;
} coll_dyn_arg_t;


typedef struct {
  snet_stream_t *output;
  snet_stream_t **inputs;
  int num;
} coll_st_arg_t;



//FIXME in threading layer?
#define SNetStreamsetIsEmpty(set) ((*set)==NULL)



/*****************************************************************************/
/* Helper functions                                                          */
/*****************************************************************************/

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
 * Get a record due to the collector setting
 */
static snet_record_t *GetRecord(
    snet_streamset_t *readyset,
    int incount,
    snet_stream_desc_t **cur_stream)
{
  assert(incount >= 1 && *readyset != NULL);
  if (*cur_stream == NULL || SNetStreamPeek(*cur_stream) == NULL) {
    if (incount == 1) {
      *cur_stream = *readyset;
    } else {
      *cur_stream = SNetStreamPoll(readyset);
    }
  }
  return SNetStreamRead(*cur_stream);
}


/**
 * Process a sort record
 */
static void ProcessSortRecord(
    snet_entity_t *ent,
    snet_record_t *rec,
    snet_record_t **sort_rec,
    snet_stream_desc_t *cur_stream,
    snet_streamset_t *readyset,
    snet_streamset_t *waitingset)
{
  int res;
  assert( REC_sort_end == SNetRecGetDescriptor( rec) );

  /* sort record: place node in waitingset */
  /* remove node */
  res = SNetStreamsetRemove( readyset, cur_stream);
  assert(res == 0);
  /* put in waiting set */
  SNetStreamsetPut( waitingset, cur_stream);

  if (*sort_rec!=NULL) {
    /*
     * check that level & counter match
     */
    if( !SortRecEqual(rec, *sort_rec) ) {
      SNetUtilDebugNoticeEnt( ent,
          "[COLL] Warning: Received sort records do not match! "
          "expected (l%d,c%d) got (l%d,c%d) on %p", /* *trollface* PROBLEM? */
          SNetRecGetLevel(*sort_rec), SNetRecGetNum(*sort_rec),
          SNetRecGetLevel(rec),       SNetRecGetNum(rec),
          cur_stream
          );
    }
    /* destroy record */
    SNetRecDestroy( rec);
  } else {
    *sort_rec = rec;
  }
}


/**
 * Process a termination record
 */
static void ProcessTermRecord(
    snet_record_t *rec,
    snet_stream_desc_t *cur_stream,
    snet_streamset_t *readyset,
    int *incount)
{
  assert( REC_terminate == SNetRecGetDescriptor( rec) );
  int res = SNetStreamsetRemove( readyset, cur_stream);
  assert(res == 0);
  SNetStreamClose( cur_stream, true);
  /* update incoming counter */
  *incount -= 1;
  /* destroy record */
  SNetRecDestroy( rec);
}


/**
 * Restore the streams from the waitingset into
 * the readyset and send sort record if required
 */
static void RestoreFromWaitingset(
    snet_streamset_t *waitingset,
    snet_streamset_t *readyset,
    snet_record_t **sort_rec,
    snet_stream_desc_t *outstream)
{
  int level;
  assert( *sort_rec != NULL);
  /* waiting set contains all streams,
     ready set is empty => "swap" sets */
  *readyset = *waitingset;
  *waitingset = NULL;
  /* check sort record: if level > 0,
     decrement and forward to outstream */
  level = SNetRecGetLevel( *sort_rec);
  if( level > 0) {
    SNetRecSetLevel( *sort_rec, level-1);
    SNetStreamWrite( outstream, *sort_rec);
  } else {
    SNetRecDestroy( *sort_rec);
  }
  *sort_rec = NULL;
}


/*****************************************************************************/
/* COLLECTOR TASKS                                                           */
/*****************************************************************************/


/**
 * Collector task for dynamic networks (star/split)
 */
void CollectorTaskDynamic(snet_entity_t *ent, void *arg)
{
  coll_dyn_arg_t *carg = (coll_dyn_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *wait_iter;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *curstream = NULL;
  snet_record_t *sort_rec = NULL;
  int incount = 1;
  bool terminate = false;


  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');

  readyset = waitingset = NULL;

  /* Open initial stream and put into readyset */
  SNetStreamsetPut( &readyset,
      SNetStreamOpen(carg->input, 'r')
      );
  /* create an iterator for waiting set, is reused within main loop*/
  wait_iter = SNetStreamIterCreate( &waitingset);

  /* MAIN LOOP */
  while( !terminate) {
    /* get a record */
    snet_record_t *rec = GetRecord(&readyset, incount, &curstream);
    /* process the record */
    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* data record: forward to output */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_sort_end:
        ProcessSortRecord(ent, rec, &sort_rec, curstream,
            &readyset, &waitingset);
        /* end processing this stream */
        curstream = NULL;
        break;

      case REC_sync:
        SNetStreamReplace( curstream, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect: /* only for dynamic collectors! */
        /* collect: add new stream to ready set */
#ifdef DESTROY_TERM_IN_WAITING_UPON_COLLECT
        /* but first, check if we can free resources by checking the
           waiting set for arrival of termination records */
        incount -= DestroyTermInWaitingSet(wait_iter, &waitingset);
        assert(incount > 0);
#endif
        /* finally, add new stream to ready set */
        SNetStreamsetPut(
            &readyset,
            SNetStreamOpen( SNetRecGetStream( rec), 'r')
            );
        /* update incoming counter */
        incount++;
        /* destroy collect record */
        SNetRecDestroy( rec);
        break;

      case REC_terminate:
        /* termination record: close stream and remove from ready set */
        ProcessTermRecord(rec, curstream, &readyset, &incount);
        /* stop processing this stream */
        curstream = NULL;
        break;

      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    } /* end switch */

    /************* termination conditions *****************/
    if ( SNetStreamsetIsEmpty( &readyset)) {
      /* the streams which had a sort record are in the waitingset */
      if ( !SNetStreamsetIsEmpty( &waitingset)) {
        RestoreFromWaitingset(&waitingset, &readyset, &sort_rec, outstream);
      } else {
        /* both ready set and waitingset are empty */
        terminate = true;
        SNetStreamWrite( outstream, SNetRecCreate( REC_terminate));
      }
    }
    /************* end of termination conditions **********/
  } /* MAIN LOOP END */

  /* close outstream */
  SNetStreamClose( outstream, false);
  /* destroy iterator */
  SNetStreamIterDestroy( wait_iter);
  SNetMemFree( carg);
} /* END of DYNAMIC COLLECTOR TASK */




/*****************************************************************************/


/**
 * Collector task for static network (parallel choice)
 */
void CollectorTaskStatic(snet_entity_t *ent, void *arg)
{
  coll_st_arg_t *carg = (coll_st_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *wait_iter;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *curstream = NULL;
  snet_record_t *sort_rec = NULL;
  snet_record_t *sync_rec = NULL;
  int incount;
  bool terminate = false;

  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');

  /* initialize input stream counter*/
  incount = carg->num;

  readyset = waitingset = NULL;

  /* open all (static) input streams */
  {
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
    /* get a record */
    snet_record_t *rec = GetRecord(&readyset, incount, &curstream);
    /* process the record */
    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* data record: forward to output */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_sort_end:
        ProcessSortRecord(ent, rec, &sort_rec, curstream, &readyset, &waitingset);
        /* end processing this stream */
        curstream = NULL;
        break;

      case REC_sync:
        {
          snet_locvec_t *source = SNetStreamGetSource( SNetRecGetStream(rec) );
          /*
           * IMPORTANT!
           *
           * If source is set, the source is a star dispatcher.
           * We must not treat the sync record as usual, otherwise
           * we might result in decrementing the level of subsequent sort
           * record which has not been incremented by the parallel dispatcher,
           * messing up the protocol completely.
           */
          if (source!=NULL) {
            /* current locvec is that of a static network,
             * source is only set at stars (dynamic network)
             */
            assert( false == SNetLocvecEqual(source, SNetEntityGetLocvec(ent)) );
            int res;
#ifdef DEBUG_PRINT_GC
            char slocvec[64];
            (void) SNetLocvecPrint(slocvec, 64, source);
            SNetUtilDebugNoticeEnt( ent, "[COLL] received REC_sync on %p with source %s!", curstream, slocvec);
#endif
            /* store the sync rec */
            assert(sync_rec == NULL);
            sync_rec = rec;
            /* remove stream from readyset */
            res = SNetStreamsetRemove( &readyset, curstream);
            assert(res == 0);
            /* close the stream */
            SNetStreamClose( curstream, false);
            incount--;
            /* stop processing this stream */
            curstream = NULL;
          } else {
            /* handle as usually */
            SNetStreamReplace( curstream, SNetRecGetStream( rec));
            SNetRecDestroy( rec);
          }
        }
        break;

      case REC_terminate:
        /* termination record: close stream and remove from ready set */
        ProcessTermRecord(rec, curstream, &readyset, &incount);
        /* stop processing this stream */
        curstream = NULL;
        break;

      default:
        assert(0);
        /* if ignore, at least destroy ... */
        SNetRecDestroy( rec);
    } /* end switch */


    /************* termination conditions *****************/
    /* check if all sort records are received */
    if ( SNetStreamsetIsEmpty( &readyset)) {
      /* the streams which had a sort record are in the waitingset */
      if ( !SNetStreamsetIsEmpty( &waitingset)) {
        RestoreFromWaitingset(&waitingset, &readyset, &sort_rec, outstream);
      } else {
        /* both ready set and waitingset are empty */
#ifdef DEBUG_PRINT_GC
        SNetUtilDebugNoticeEnt( ent,
            "[COLL] Terminate static collector as no inputs left!%s",
            (sync_rec != NULL) ? " (sending sync)" : ""
            );
#endif
        terminate = true;
        if (sync_rec != NULL) {
          /* There is a sync record pending that needs to be forwarded */
          SNetStreamWrite( outstream, sync_rec);
          sync_rec = NULL;
        } else {
          SNetStreamWrite( outstream, SNetRecCreate( REC_terminate));
        }
      }
    } else if (incount==1 && sync_rec==NULL) {
      /* static collector has only one incoming stream left
       * and no pending sync record
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
#ifdef DEBUG_PRINT_GC
      /* terminating due to GC */
      SNetUtilDebugNoticeEnt( ent,
          "[COLL] Terminate static collector as only one branch left!"
          );
#endif
    }
    /************* end of termination conditions **********/
  } /* MAIN LOOP END */

  assert(sync_rec == NULL);
  /* close outstream */
  SNetStreamClose( outstream, false);
  /* destroy iterator */
  SNetStreamIterDestroy( wait_iter);
  /* destroy argument */
  SNetMemFree( carg);
} /* END of STATIC COLLECTOR TASK */




/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/



/**
 * Collector creation function
 * @pre num >= 1
 */
snet_stream_t *CollectorCreateStatic( int num, snet_stream_t **instreams, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_st_arg_t *carg;
  int i;

  assert(num >= 1);
  /* create outstream */
  outstream =  SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_st_arg_t *) SNetMemAlloc( sizeof( coll_st_arg_t));
  /* copy instreams */
  carg->inputs = SNetMemAlloc(num * sizeof(snet_stream_t *));
  for(i=0; i<num; i++) {
    ((snet_stream_t **)carg->inputs)[i] = instreams[i];
  }
  carg->output = outstream;
  carg->num = num;

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", CollectorTaskStatic, (void*)carg)
      );
  return outstream;
}



/**
 * Collector creation function
 */
snet_stream_t *CollectorCreateDynamic( snet_stream_t *instream, int location, snet_info_t *info)
{
  snet_stream_t *outstream;
  coll_dyn_arg_t *carg;

  /* create outstream */
  outstream = SNetStreamCreate(0);

  /* create collector handle */
  carg = (coll_dyn_arg_t *) SNetMemAlloc( sizeof( coll_dyn_arg_t));
  carg->input = instream;
  carg->output = outstream;

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", CollectorTaskDynamic, (void*)carg)
      );
  return outstream;
}

