#include <stdlib.h>
#include <assert.h>

#include "snetentities.h"
#include "record_p.h"

#include "memfun.h"
#include "hashtab.h"
#include "collector.h"
#include "lpelif.h"
#include "lpel.h"

#include "distribution.h"

/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */


typedef struct {
  lpel_stream_t *input, *output;
  snet_startup_fun_t boxfun;
  snet_info_t *info;
  int ltag, utag;
  bool is_det;
  bool is_byloc;
} split_arg_t;

/**
 * Split box task.
 *
 * Implements both the non-deterministic and deterministic variants.
 */
static void SplitBoxTask( lpel_task_t *self, void *arg)
{
  int i;
  split_arg_t *sarg = (split_arg_t *)arg;
  lpel_stream_desc_t *initial, *instream;
  int ltag_val, utag_val;
  snet_record_t *rec;
  bool terminate = false;
  /* a list of all outstreams for all yet created instances */
  lpel_stream_list_t repos_list = NULL;
  lpel_stream_iter_t *iter = LpelStreamIterCreate( &repos_list);
  /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
  hashtab_t *repos_tab = HashtabCreate( 4);

  /* for deterministic variant: */
  int counter = 0;

  initial = LpelStreamOpen( self, sarg->output, 'w');
  instream = LpelStreamOpen( self, sarg->input, 'r');

  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = LpelStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* get lower and upper tag values */
        ltag_val = SNetRecGetTag( rec, sarg->ltag);
        utag_val = SNetRecGetTag( rec, sarg->utag);

        /* for all tag values */
        for( i = ltag_val; i <= utag_val; i++) {
          lpel_stream_desc_t *outstream = HashtabGet( repos_tab, i);

          if( outstream == NULL) {
            snet_stream_t *newstream_addr = (snet_stream_t*) LpelStreamCreate();
            /* instance does not exist yet, create it */
            outstream = LpelStreamOpen( self, (lpel_stream_t*) newstream_addr, 'w');
            /* add to lookup table */
            HashtabPut( repos_tab, i, outstream);
            /* add to list */
            LpelStreamListAppend( &repos_list, outstream);

            snet_stream_t *temp_stream;
            if( sarg->is_byloc) {
              temp_stream = sarg->boxfun(newstream_addr, sarg->info, i);
            } else {
              temp_stream = sarg->boxfun(newstream_addr, sarg->info, SNetNodeLocation);
            }

            if(temp_stream != NULL) {
              /* notify collector about the new instance via initial */
              LpelStreamWrite( initial,
                  SNetRecCreate( REC_collect, temp_stream));
            }
          } /* end if (outstream==NULL) */

          /* multicast the record */
          LpelStreamWrite( outstream,
              /* copy record for all but the last tag value */
              (i!=utag_val) ? SNetRecCopy( rec) : rec
              );
        } /* end for all tags  ltag_val <= i <= utag_val */

        /* If deterministic, append a sort record to *all* registered
         * instances and the initial stream.
         */
        if( sarg->is_det ) {
          /* reset iterator */
          LpelStreamIterReset( &repos_list, iter);
          while( LpelStreamIterHasNext( iter)) {
            lpel_stream_desc_t *cur_stream = LpelStreamIterNext( iter);

            LpelStreamWrite( cur_stream,
                SNetRecCreate( REC_sort_end, 0, counter));
          }
          /* Now also send a sort record to initial,
             after the collect records for new instances have been sent */
          LpelStreamWrite( initial,
              SNetRecCreate( REC_sort_end, 0, counter));
        }
        /* increment counter for deterministic variant */
        counter += 1;
        break;

      case REC_sync:
        {
          lpel_stream_t *newstream = (lpel_stream_t *) SNetRecGetStream( rec);
          LpelStreamReplace( instream, newstream);
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
        LpelStreamIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( LpelStreamIterHasNext( iter)) {
          lpel_stream_desc_t *cur_stream = LpelStreamIterNext( iter);
          LpelStreamWrite( cur_stream,
              SNetRecCreate( REC_sort_end,
                /* we have to increase level */
                SNetRecGetLevel( rec)+1,
                SNetRecGetNum( rec))
              );
        }
        /* send the original record to the initial stream,
           but with increased level */
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        LpelStreamWrite( initial, rec);
        break;

      case REC_terminate:

        LpelStreamIterReset( &repos_list, iter);
        /* all instances receive copies of the record */
        while( LpelStreamIterHasNext( iter)) {
          lpel_stream_desc_t *cur_stream = LpelStreamIterNext( iter);
          LpelStreamWrite( cur_stream, SNetRecCopy( rec));

          LpelStreamIterRemove( iter);
          /* close  the stream to the instance */
          LpelStreamClose( cur_stream, false);
        }
        /* send the original record to the initial stream */
        LpelStreamWrite( initial, rec);
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
  LpelStreamIterDestroy( iter);

  /* close and destroy initial stream */
  LpelStreamClose( initial, false);
  /* close instream */
  LpelStreamClose( instream, true);

  /* free info object */
  SNetInfoDestroy(sarg->info);

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
snet_stream_t *CreateSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag,
    bool is_byloc,
    bool is_det
    )
{
  snet_stream_t **initial, *output;
  split_arg_t *sarg;

  input = SNetRouteUpdate(info, input, location);
  if(location == SNetNodeLocation) {
    initial = (snet_stream_t **) SNetMemAlloc(sizeof(lpel_stream_t *));
    initial[0] = (snet_stream_t*) LpelStreamCreate();
    sarg = (split_arg_t *) SNetMemAlloc( sizeof( split_arg_t));
    sarg->input  = (lpel_stream_t*) input;
    sarg->output = (lpel_stream_t*) initial[0];
    sarg->boxfun = box_a;
    sarg->info = SNetInfoCopy(info);
    sarg->ltag = ltag;
    sarg->utag = utag;
    sarg->is_det = is_det;
    sarg->is_byloc = is_byloc;

    output = CollectorCreate( 1, initial, info);
    SNetLpelIfSpawnEntity( SplitBoxTask, (void*)sarg,
        (is_det) ? ENTITY_split_det : ENTITY_split_nondet,
        NULL
        );

  } else {
    output = input;
  }

  return( output);
}




/**
 * Non-det Split creation function
 *
 */
snet_stream_t *SNetSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  return CreateSplit( input, info, location, box_a, ltag, utag,
      false, /* not by location */
      false  /* not det */
      );
}



/**
 * Det Split creation function
 *
 */
snet_stream_t *SNetSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  return CreateSplit( input, info, location, box_a, ltag, utag,
      false, /* not by location */
      true   /* is det */
      );
}

/**
 * Non-det Location Split creation function
 *
 */
snet_stream_t *SNetLocSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  return CreateSplit( input, info, location, box_a, ltag, utag,
      true, /* is by location */
      false /* not det */
      );
}

/**
 * Det Location Split creation function
 *
 */
snet_stream_t *SNetLocSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  return CreateSplit( input, info, location, box_a, ltag, utag,
      true, /* is by location */
      true  /* is det */
      );
}
