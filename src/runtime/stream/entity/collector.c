#include <stdlib.h>
#include <assert.h>

#include "collector.h"
#include "snetentities.h"
#include "locvec.h"
#include "memfun.h"
#include "bool.h"
#include "debug.h"

#include "threading.h"


#define LIST_NAME_H Stream
#define LIST_TYPE_NAME_H stream
#define LIST_VAL_H snet_stream_t *
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

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
  snet_stream_desc_t *curstream, *outstream;
  snet_record_t *sort_rec;
  snet_record_t *term_rec;
  snet_streamset_t readyset, waitingset;
  int incount;
  bool is_static;
} coll_arg_t;

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
      SNetUtilDebugNoticeTask(
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
    snet_record_t **term_rec,
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

  if (*term_rec == NULL) {
    *term_rec = rec;
  } else {
    /* destroy record */
    SNetRecDestroy( rec);
  }
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


static void TerminateCollectorTask(snet_stream_desc_t *outstream,
                                   coll_arg_t *carg)
{
  if (carg->term_rec != NULL) {
    SNetRecDestroy(carg->term_rec);
  }
  if (carg->sort_rec != NULL) {
    SNetRecDestroy(carg->sort_rec);
  }

  /* close outstream */
  SNetStreamClose( outstream, false);

  /* destroy argument */
  SNetMemFree( carg);
}

/*****************************************************************************/
/* COLLECTOR TASK                                                            */
/*****************************************************************************/

static void CollectorTask(void *arg)
{
  coll_arg_t *carg = arg;
  snet_stream_iter_t *wait_iter;

  /* create an iterator for waiting set, is reused within main loop*/
  wait_iter = SNetStreamIterCreate( &carg->waitingset);

  /* get a record */
  snet_record_t *rec = GetRecord(&carg->readyset, carg->incount, &carg->curstream);
  /* process the record */
  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* data record: forward to output */
      SNetStreamWrite( carg->outstream, rec);
      break;

    case REC_sort_end:
      ProcessSortRecord(rec, &carg->sort_rec, carg->curstream, &carg->readyset, &carg->waitingset);
      /* end processing this stream */
      carg->curstream = NULL;
      break;

    case REC_sync:
      SNetStreamReplace( carg->curstream, SNetRecGetStream( rec));
      SNetRecDestroy( rec);
      break;

    case REC_collect:
      /* only for dynamic collectors! */
      assert( false == carg->is_static );
      /* collect: add new stream to ready set */
#ifdef DESTROY_TERM_IN_WAITING_UPON_COLLECT
      /* but first, check if we can free resources by checking the
         waiting set for arrival of termination records */
      carg->incount -= DestroyTermInWaitingSet(wait_iter, &carg->waitingset);
      assert(carg->incount > 0);
#endif
      /* finally, add new stream to ready set */
      SNetStreamsetPut(
          &carg->readyset,
          SNetStreamOpen( SNetRecGetStream( rec), 'r')
          );
      /* update incoming counter */
      carg->incount++;
      /* destroy collect record */
      SNetRecDestroy( rec);
      break;

    case REC_terminate:
      /* termination record: close stream and remove from ready set */
      ProcessTermRecord(rec, &carg->term_rec, carg->curstream, &carg->readyset, &carg->incount);
      /* stop processing this stream */
      carg->curstream = NULL;
      break;

    default:
      assert(0);
      /* if ignore, at least destroy ... */
      SNetRecDestroy( rec);
  } /* end switch */

  /************* termination conditions *****************/
  if ( SNetStreamsetIsEmpty( &carg->readyset)) {
    /* the streams which had a sort record are in the waitingset */
    if ( !SNetStreamsetIsEmpty( &carg->waitingset)) {
      /* check incount */
      if ( carg->is_static && (1 == carg->incount) ) {
        /* static collector has only one incoming stream left
         * -> this input stream can be sent to the collectors
         *    successor via a sync record
         * -> collector can terminate
         */
        snet_stream_desc_t *in = carg->waitingset;

        SNetStreamWrite( carg->outstream,
            SNetRecCreate( REC_sync, SNetStreamGet(in))
            );
        SNetStreamClose( in, false);
#ifdef DEBUG_PRINT_GC
        /* terminating due to GC */
        SNetUtilDebugNoticeEnt( ent,
            "[COLL] Terminate static collector as only one branch left!"
            );
#endif
        TerminateCollectorTask(carg->outstream, carg);
        SNetStreamIterDestroy(wait_iter);
        return;
      } else {
        RestoreFromWaitingset(&carg->waitingset,
                              &carg->readyset,
                              &carg->sort_rec,
                              carg->outstream);
      }
    } else {
      /* both ready set and waitingset are empty */
#ifdef DEBUG_PRINT_GC
      if (carg->is_static) {
        SNetUtilDebugNoticeEnt( ent,
            "[COLL] Terminate static collector as no inputs left!");
      }
#endif
      assert(carg->term_rec != NULL);
      SNetStreamWrite( carg->outstream, carg->term_rec);
      carg->term_rec = NULL;
      TerminateCollectorTask(carg->outstream, carg);
      SNetStreamIterDestroy(wait_iter);
      return;
    }
  }
  /************* end of termination conditions **********/

  SNetStreamIterDestroy(wait_iter);
  SNetThreadingRespawn(NULL);
}

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
  coll_arg_t *carg;
  int i;

  assert(num >= 1);
  /* create outstream */
  outstream =  SNetStreamCreate(0);

  /* create collector handle */
  carg = SNetMemAlloc( sizeof( coll_arg_t));
  carg->outstream = SNetStreamOpen(outstream, 'w');
  carg->is_static = true;
  carg->incount = num;
  carg->curstream = NULL;

  carg->readyset = NULL;
  carg->waitingset = NULL;
  carg->sort_rec = NULL;
  carg->term_rec = NULL;

  /* copy instreams */
  for(i=0; i<num; i++) {
    SNetStreamsetPut(&carg->readyset, SNetStreamOpen(instreams[i], 'r'));
  }


  /* spawn collector task */
  SNetThreadingSpawn( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", &CollectorTask, carg);
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
  outstream = SNetStreamCreate(0);

  /* create collector handle */
  carg = SNetMemAlloc( sizeof( coll_arg_t));
  carg->curstream = SNetStreamOpen(instream, 'r');
  carg->outstream = SNetStreamOpen(outstream, 'w');
  carg->is_static = false;
  carg->incount = 1;

  carg->readyset = NULL;
  carg->waitingset = NULL;
  carg->sort_rec = NULL;
  carg->term_rec = NULL;

  SNetStreamsetPut(&carg->readyset, carg->curstream);

  /* spawn collector task */
  SNetThreadingSpawn( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", &CollectorTask, carg);
  return outstream;
}
