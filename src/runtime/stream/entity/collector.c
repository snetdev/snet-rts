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
  bool is_static;
  union {
    struct {
      int num;
      snet_stream_t **inputs;
    } st;
    struct {
      snet_stream_t *input;
    } dyn;
  } in;
} coll_arg_t;

#define CARG_ST( carg,name) (carg->in.st.name)
#define CARG_DYN(carg,name) (carg->in.dyn.name)


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


/*****************************************************************************/
/* COLLECTOR TASK                                                            */
/*****************************************************************************/


/**
 * Collector task for dynamic combinators (star/split)
 * and the static parallel combinator
 */
void CollectorTask(snet_entity_t *ent, void *arg)
{
  coll_arg_t *carg = (coll_arg_t *)arg;
  snet_streamset_t readyset, waitingset;
  snet_stream_iter_t *wait_iter;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *curstream = NULL;
  snet_stream_desc_t *last = NULL;		// when the paired parallel terminate, it sends the sort_end record to the last branch
  snet_record_t *sort_rec = NULL;
  snet_record_t *term_rec = NULL;
  int incount;
  bool terminate = false;


  /* open outstream for writing */
  outstream = SNetStreamOpen(carg->output, 'w');

  readyset = waitingset = NULL;

  if (carg->is_static) {
    int i;
    incount = CARG_ST(carg, num);
    /* fill initial readyset of collector */
    for (i=0; i<incount; i++) {
      snet_stream_desc_t *tmp;
      /* open each stream in listening set for reading */
      tmp = SNetStreamOpen( CARG_ST(carg, inputs[i]), 'r');
      /* add each stream instreams[i] to listening set of collector */
      SNetStreamsetPut( &readyset, tmp);
    }
    SNetMemFree( CARG_ST(carg, inputs) );
  } else {
    incount = 1;
    /* Open initial stream and put into readyset */
    SNetStreamsetPut( &readyset,
        SNetStreamOpen(CARG_DYN(carg, input), 'r')
        );
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
      	/* curstream == last, this is the last branch and the paired parallel already terminates
      	 * increase the level by one because it was not increased by the paired parallel as it should be
      	 * Also later when last becomes the only waiting branch, the collector should terminate. However before terminating, it should pretend that parallel has sent the sort end from all branches
      	 */
      	if (curstream == last) {			// imply last != NULL, this will be the last branch and the paired parallel already terminates
      		SNetRecSetLevel(rec, SNetRecGetLevel(rec) + 1);			// increase the level by one because it was not increased by the paired parallel as it should be
      	}
      	if (last == NULL && SNetRecGetLevel(rec) == 0 && SNetRecGetNum(rec) == -1) {		// if last was not set, and collector receives a sort_end (l0, c-1) --> set the curstream as last
      		last = curstream;																															// from now on, any sort_end from last will be increased level by 1
      		SNetRecDestroy( rec);
      		break;			// ignore the sort_end
      	}

        ProcessSortRecord(ent, rec, &sort_rec, curstream, &readyset, &waitingset);
        /* end processing this stream */
        curstream = NULL;
        break;

      case REC_sync:
        SNetStreamReplace( curstream, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        /* only for dynamic collectors! */
        assert( false == carg->is_static );
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
        ProcessTermRecord(rec, &term_rec, curstream, &readyset, &incount);
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
        if ( carg->is_static && (1 == incount) ) {		// stat
          snet_stream_desc_t *in = (waitingset != NULL) ? waitingset : readyset;

          /* if last is the only one in the waitingset --> pretends that the already-terminated paired parallel has sent the sort end to all branches
           * Therefore restore the waitingset before terminating (so that a relevant sort_end is sent out) */
          if (in == last)
          	RestoreFromWaitingset(&waitingset, &readyset, &sort_rec, outstream);

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
        } else
          RestoreFromWaitingset(&waitingset, &readyset, &sort_rec, outstream);
      } else {
        /* both ready set and waitingset are empty */
#ifdef DEBUG_PRINT_GC
        if (carg->is_static) {
          SNetUtilDebugNoticeEnt( ent,
              "[COLL] Terminate static collector as no inputs left!");
        }
#endif
        assert(term_rec != NULL);
        SNetStreamWrite( outstream, term_rec);
        term_rec = NULL;
        terminate = true;
      }
    }

     
    /************* end of termination conditions **********/
  } /* MAIN LOOP END */

  if (term_rec != NULL) {
    SNetRecDestroy(term_rec);
  }
  if (sort_rec != NULL) {
    SNetRecDestroy(sort_rec);
  }
  /* close outstream */
  SNetStreamClose( outstream, false);
  /* destroy iterator */
  SNetStreamIterDestroy( wait_iter);
  /* destroy argument */
  SNetMemFree( carg);
} /* END of DYNAMIC COLLECTOR TASK */



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
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->output = outstream;
  carg->is_static = true;
  CARG_ST(carg, num) = num;
  /* copy instreams */
  CARG_ST(carg, inputs) = SNetMemAlloc(num * sizeof(snet_stream_t *));
  for(i=0; i<num; i++) {
    CARG_ST(carg, inputs[i]) = instreams[i];
  }

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", CollectorTask, (void*)carg)
      );
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
  carg = (coll_arg_t *) SNetMemAlloc( sizeof( coll_arg_t));
  carg->output = outstream;
  carg->is_static = false;
  CARG_DYN(carg, input) = instream;

  /* spawn collector task */
  SNetThreadingSpawn(
      SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
        "<collect>", CollectorTask, (void*)carg)
      );
  return outstream;
}

