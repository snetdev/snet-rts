#include <stdlib.h>
#include <assert.h>

#include "snetentities.h"
#include "record_p.h"

#include "memfun.h"
#include "collector.h"
#include "spawn.h"

#include "stream.h"
#include "task.h"
#include "hashtab.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#include "fun.h"
#include "debug.h"
#endif /* DISTRIBUTED_SNET */

/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */


typedef struct {
  stream_t *input, *output;
  snet_startup_fun_t boxfun;
  int ltag, utag;
  bool is_det;
  bool is_byloc;
} split_arg_t;

/**
 * Split box task.
 *
 * Implements both the non-deterministic and deterministic variants.
 */
static void SplitBoxTask( task_t *self, void *arg)
{
  int i;
  split_arg_t *sarg = (split_arg_t *)arg;
  stream_desc_t *initial, *instream;
  int ltag_val, utag_val;
  snet_record_t *rec;
  bool terminate = false;
  /* a list of all outstreams for all yet created instances */
  stream_list_t repos_list = NULL;
  stream_iter_t *iter = StreamIterCreate( &repos_list);
  /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
  hashtab_t *repos_tab = HashtabCreate( 4);

  /* for deterministic variant: */
  int counter = 0;

#ifdef DISTRIBUTED_SNET
  int node_id;
  snet_fun_id_t fun_id;
#endif /* DISTRIBUTED_SNET */

  initial = StreamOpen( self, sarg->output, 'w');


  instream = StreamOpen( self, sarg->input, 'r');

#ifdef DISTRIBUTED_SNET
  node_id = SNetIDServiceGetNodeID();
  if(!SNetDistFunFun2ID(sarg->boxfun, &fun_id)) {
    /* TODO: This is an error!*/
  }
#endif /* DISTRIBUTED_SNET */


  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = StreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* get lower and upper tag values */
        ltag_val = SNetRecGetTag( rec, sarg->ltag);
        utag_val = SNetRecGetTag( rec, sarg->utag);
        
        /* for all tag values */
        for( i = ltag_val; i <= utag_val; i++) {
          stream_desc_t *outstream = HashtabGet( repos_tab, i);

          if( outstream == NULL) {
            stream_t *newstream_addr = StreamCreate();
            /* instance does not exist yet, create it */
            outstream = StreamOpen( self, newstream_addr, 'w');
            /* add to lookup table */
            HashtabPut( repos_tab, i, outstream);
            /* add to list */
            StreamListAppend( &repos_list, outstream);
            {
              snet_info_t *info = SNetInfoInit();
#ifdef DISTRIBUTED_SNET
              stream_t *temp_stream;
              if( sarg->is_byloc) {
                SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, i));    
                temp_stream = sarg->boxfun(newstream_addr, info, i);    
                temp_stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), temp_stream);
              } else {
                SNetInfoSetRoutingContext(info, SNetRoutingContextInit(SNetRoutingGetNewID(), true, node_id, &fun_id, node_id));
                temp_stream = sarg->boxfun(newstream_addr, info, node_id);    
                temp_stream = SNetRoutingContextEnd(SNetInfoGetRoutingContext(info), temp_stream);
              }
              if(temp_stream != NULL) {
                /* notify collector about the new instance via initial */
                StreamWrite( initial,
                    SNetRecCreate( REC_collect, temp_stream));
              }
#else
              /* notify collector about the new instance via initial */
              StreamWrite( initial, 
                  SNetRecCreate( REC_collect, sarg->boxfun( newstream_addr, info)));
#endif /* DISTRIBUTED_SNET */
              SNetInfoDestroy(info);
            }
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
        if( sarg->is_det ) {
          /* reset iterator */
          StreamIterReset( &repos_list, iter);
          while( StreamIterHasNext( iter)) {
            stream_desc_t *cur_stream = StreamIterNext( iter);

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
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        /* invalid control record */
        assert( 0);
        /* if to ignore, at least destroy it ...*/
        SNetRecDestroy( rec);
        break;

      case REC_sort_end:
        /* broadcast the sort record */
        StreamIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( StreamIterHasNext( iter)) {
          stream_desc_t *cur_stream = StreamIterNext( iter);
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

        StreamIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( StreamIterHasNext( iter)) {
          stream_desc_t *cur_stream = StreamIterNext( iter);
          StreamWrite( cur_stream, SNetRecCopy( rec));

          StreamIterRemove( iter);
          /* close  the stream to the instance */
          StreamClose( cur_stream, false);
        }
        /* send the original record to the initial stream */
        StreamWrite( initial, rec);
        /* note that no sort record has to be appended */
        terminate = true;
        break;

      default:
        assert( 0);
        /* if ignore, at least destroy it */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  /* destroy repository */
  HashtabDestroy( repos_tab);
  StreamIterDestroy( iter);

  /* close and destroy initial stream */
  StreamClose( initial, false);
  /* close instream */
  StreamClose( instream, true);

  /* destroy the argument */
  SNetMemFree( sarg);
} /* END of SPLIT BOX TASK */




/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/


/**
 * Convenience function for creating
 * Split, DetSplit, LocSplit or LocSplitDet,
 * dependent on parameters is_byloc and is_det
 */
stream_t *CreateSplit( stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, int utag,
    bool is_byloc,
    bool is_det
    ) 
{
  stream_t **initial, *output;
  split_arg_t *sarg;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    if (is_byloc) {
      SNetUtilDebugNotice( (is_det) ? "DetLocSplit created" : "LocSplit created");
    } else {
      SNetUtilDebugNotice( (is_det) ? "DetSplit created" : "Split created");
    }
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    
    initial = (stream_t **) SNetMemAlloc(sizeof(stream_t *));
    initial[0] = StreamCreate();
    sarg = (split_arg_t *) SNetMemAlloc( sizeof( split_arg_t));
    sarg->input = input;
    sarg->output = initial[0];
    sarg->boxfun = box_a;
    sarg->ltag = ltag;
    sarg->utag = utag;
    sarg->is_det = is_det;
    sarg->is_byloc = is_byloc;

    output = CollectorCreate( 1, initial, info);
    SNetSpawnEntity( SplitBoxTask, (void*)sarg,
        (is_det) ? ENTITY_split_det : ENTITY_split_nondet
        );
    
#ifdef DISTRIBUTED_SNET
  } else { 
    output = input; 
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}




/**
 * Non-det Split creation function
 *
 */
stream_t *SNetSplit( stream_t *input,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, int utag) 
{
  return CreateSplit( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      box_a, ltag, utag,
      false, /* not by location */
      false  /* not det */
      );
}



/**
 * Det Split creation function
 *
 */
stream_t *SNetSplitDet( stream_t *input, 
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, int utag) 
{
  return CreateSplit( input, info,
#ifdef DISTRIBUTED_SNET
      location,
#endif /* DISTRIBUTED_SNET */
      box_a, ltag, utag,
      false, /* not by location */
      true   /* is det */
      );
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
  return CreateSplit( input, info,
      location, box_a, ltag, utag,
      true, /* is by location */
      false /* not det */
      );
}

/**
 * Det Location Split creation function
 *
 */
stream_t *SNetLocSplitDet( stream_t *input,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  return CreateSplit( input, info,
      location, box_a, ltag, utag,
      true, /* is by location */
      true  /* is det */
      );
} 
#endif /* DISTRIBUTED_SNET */

