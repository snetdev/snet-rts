#include <stdlib.h>

#include "split.h"

#include "memfun.h"
#include "buffer.h"
#include "handle.h"
#include "debug.h"
#include "collectors.h"
#include "threading.h"

#include "linklst.h"
#include "hashtab.h"

#include "stream.h"
#include "task.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#include "fun.h"
#endif /* DISTRIBUTED_SNET */

/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */


/**
 * Split box task.
 *
 * Implements both the non-deterministic and deterministic variants.
 */
static void SplitBoxTask( task_t *self, void *arg)
{
  int i;
  snet_handle_t *hnd = (snet_handle_t*)arg;
  stream_mh_t *initial, *instream;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec;
  snet_startup_fun_t boxfun;
  bool terminate = false;
  /* a list of all outstreams for all yet created instances */
  list_hnd_t repos_list = NULL;
  list_iter_t *iter = ListIterCreate( &repos_list);
  /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
  hashtab_t *repos_tab = HashtabCreate( 4);

  bool is_det;
  /* for deterministic variant: */
  int counter = 0;

#ifdef DISTRIBUTED_SNET
  int node_id;
  snet_fun_id_t fun_id;
#endif /* DISTRIBUTED_SNET */

  initial = StreamOpen( self, SNetHndGetOutput( hnd), 'w');

  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);
  is_det = SNetHndIsDet( hnd);

  instream = StreamOpen( self, SNetHndGetInput( hnd), 'r');

#ifdef DISTRIBUTED_SNET
  node_id = SNetIDServiceGetNodeID();
  if(!SNetDistFunFun2ID(boxfun, &fun_id)) {
    /* TODO: This is an error!*/
  }
#endif /* DISTRIBUTED_SNET */


  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = StreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
#ifdef DEBUG_SPLIT
        SNetUtilDebugNotice("SPLIT got data");
#endif
        /* get lower and upper tag values */
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        
        /* for all tag values */
        for( i = ltag_val; i <= utag_val; i++) {
          stream_mh_t *outstream = HashtabGet( repos_tab, i);

          if( outstream == NULL) {
            stream_t *newstream_addr = StreamCreate();
            /* instance does not exist yet, create it */
            outstream = StreamOpen( self, newstream_addr, 'w');
            /* add to lookup table */
            HashtabPut( repos_tab, i, outstream);
            /* add to list */
            ListAppend( &repos_list, ListNodeCreate( outstream));
#ifdef DISTRIBUTED_SNET
            {
              snet_info_t *info;
              stream_t *temp_stream;
              info = SNetInfoInit();
              if(SNetHndIsSplitByLocation( hnd)) {
                SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, i));    
                temp_stream = boxfun(newstream_addr, info, i);    
                temp_stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), temp_stream);
              } else {
                SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, node_id));
                temp_stream = boxfun(newstream_addr, info, node_id);    
                temp_stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), temp_stream);
              }
              if(temp_stream != NULL) {
                /* notify collector about the new instance via initial */
                StreamWrite( initial,
                    SNetRecCreate( REC_collect, temp_stream));
              }
              SNetInfoDestroy(info);
            }
#else
            /* notify collector about the new instance via initial */
            StreamWrite( initial, 
                SNetRecCreate( REC_collect, boxfun( newstream_addr)));
#endif /* DISTRIBUTED_SNET */
          } /* end if (outstream==NULL) */

          /* multicast the record */
          StreamWrite( outstream,
              /* copy record for all but the last tag value */
              (i!=utag_val) ? SNetRecCopy( rec) : rec
              );
        } /* end for all tags  ltag_val <= i <= utag_val */

        /* If deterministic, append a sort record to *all* registered
         * instances and the initial stream.
         */
        if( is_det ) {
          /* reset iterator */
          ListIterReset( &repos_list, iter);
          while( ListIterHasNext( iter)) {
            stream_mh_t *cur_stream = ListNodeGet( ListIterNext( iter));

            StreamWrite( cur_stream,
                SNetRecCreate( REC_sort_end, 0, counter));
          }
          /* Now also send a sort record to initial,
             after the collect records for new instances have been sent */
          StreamWrite( initial,
              SNetRecCreate( REC_sort_end, 0, counter));
        }
        /* increment counter for deterministic variant */
        counter += 1;
        break;

      case REC_sync:
        {
          stream_t *newstream = SNetRecGetStream( rec);
          StreamReplace( instream, newstream);
          SNetHndSetInput( hnd, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
#ifdef DEBUG_SPLIT
        SNetUtilDebugNotice("[SPLIT] Invalid control record (REC_collect),"
            " destroying it \n\n");
#endif
        SNetRecDestroy( rec);
        break;

      case REC_sort_end:
        /* broadcast the sort record */
        ListIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( ListIterHasNext( iter)) {
          stream_mh_t *cur_stream = ListNodeGet( ListIterNext( iter));
          StreamWrite( cur_stream,
              SNetRecCreate( REC_sort_end,
                /* we have to increase level */
                SNetRecGetLevel( rec)+1,
                SNetRecGetNum( rec))
              );
        }
        /* send the original record to the initial stream,
           but with increased level */
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        StreamWrite( initial, rec);
        break;

      case REC_terminate:

        ListIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( ListIterHasNext( iter)) {
          list_node_t *cur_node = ListIterNext( iter);
          stream_mh_t *cur_stream = ListNodeGet( cur_node);
          StreamWrite( cur_stream, SNetRecCopy( rec));

          ListIterRemove( iter);
          /* close  the stream to the instance */
          StreamClose( cur_stream, false);
          ListNodeDestroy( cur_node);
        }
        /* send the original record to the initial stream */
        StreamWrite( initial, rec);
        /* note that no sort record has to be appended */
        terminate = true;
        break;

      default:
        SNetUtilDebugNotice("[Split] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
        SNetRecDestroy( rec);
        break;
    }
  } /* MAIN LOOP END */

  /* destroy repository */
  HashtabDestroy( repos_tab);
  ListIterDestroy( iter);

  /* close and destroy initial stream */
  StreamClose( initial, false);
  /* close instream */
  StreamClose( instream, true);
  /* destroy the handle */
  SNetHndDestroy( hnd);
} /* END of SPLIT BOX TASK */



/**
 * Non-det Split creation function
 *
 */
stream_t *SNetSplit( stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, int utag) 
{
  stream_t **initial, *output;
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Split created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    
    initial = (stream_t **) SNetMemAlloc(sizeof(stream_t *));
    initial[0] = StreamCreate();
    hnd = SNetHndCreate( HND_split, input, initial[0], box_a, ltag, utag, false, false);
    output = CollectorCreate( 1, initial);
    SNetEntitySpawn( SplitBoxTask, (void*)hnd, ENTITY_split_nondet);
    
#ifdef DISTRIBUTED_SNET
  } else { 
    output = input; 
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}





#ifdef DISTRIBUTED_SNET
/**
 * Non-det Location Split creation function
 *
 */
stream_t *SNetLocSplit( stream_t *input,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  stream_t **initial, *output;
  snet_handle_t *hnd;

  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("LocSplit created");
#endif /* DISTRIBUTED_DEBUG */

    initial = (stream_t **) SNetMemAlloc(sizeof(stream_t *));
    initial[0] = StreamCreate();
    hnd = SNetHndCreate( HND_split, input, initial[0], box_a, ltag, utag, false, true);
    output = CollectorCreate( 1, initial);
    SNetEntitySpawn( SplitBoxTask, (void*)hnd, ENTITY_split_nondet);
  } else { 
    output = input; 
  }
  return( output);
}
#endif /* DISTRIBUTED_SNET */




/**
 * Det Split creation function
 *
 */
stream_t *SNetSplitDet( stream_t *input, 
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, int utag) 
{
  stream_t **initial, *output;
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetSplit created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    
    initial = (stream_t **) SNetMemAlloc(sizeof(stream_t *));
    initial[0] = StreamCreate();
    hnd = SNetHndCreate( HND_split, input, initial[0], box_a, ltag, utag, true, false);
    output = CollectorCreate( 1, initial);
    SNetEntitySpawn( SplitBoxTask, (void*)hnd, ENTITY_split_det);

#ifdef DISTRIBUTED_SNET
  } else { 
    output = input; 
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}




/**
 * Det Location Split creation function
 *
 */
#ifdef DISTRIBUTED_SNET
stream_t *SNetLocSplitDet( stream_t *input,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  stream_t **initial, *output;
  snet_handle_t *hnd;

  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 							  
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("DetLocSplit created");
#endif /* DISTRIBUTED_DEBUG */

    initial = (stream_t **) SNetMemAlloc(sizeof(stream_t *));
    initial[0] = StreamCreate();
    hnd = SNetHndCreate( HND_split, input, initial[0], box_a, ltag, utag, true, true);
    output = CollectorCreate( 1, initial);
    SNetEntitySpawn( SplitBoxTask, (void*)hnd, ENTITY_split_det);
  } else { 
    output = input; 
  }
  return( output);
}
#endif /* DISTRIBUTED_SNET */
 
