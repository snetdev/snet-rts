#include <stdlib.h>
#include <assert.h>

#include "snetentities.h"

#include "memfun.h"
#include "locvec.h"
#include "hashtab.h"
#include "collector.h"
#include "threading.h"

#include "distribution.h"

/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */


typedef struct {
  snet_stream_t *input, *output;
  snet_startup_fun_t boxfun;
  snet_info_t *info;
  int ltag, utag;
  bool is_det;
  bool is_byloc;
  int location;
} split_arg_t;

/**
 * Split box task.
 *
 * Implements both the non-deterministic and deterministic variants.
 */
static void SplitBoxTask(snet_entity_t *ent, void *arg)
{
  int i;
  split_arg_t *sarg = (split_arg_t *)arg;
  snet_stream_desc_t *initial, *instream;
  int ltag_val, utag_val;
  snet_info_t *info;
  snet_record_t *rec;
  snet_locvec_t *locvec;
  bool terminate = false;
  /* a list of all outstreams for all yet created instances */
  snet_streamset_t repos_set = NULL;
  snet_stream_iter_t *iter = SNetStreamIterCreate( &repos_set);
  /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
  hashtab_t *repos_tab = HashtabCreate( 4);
  (void) ent; /* NOT USED */

  /* for deterministic variant: */
  int counter = 0;

  initial = SNetStreamOpen(sarg->output, 'w');
  instream = SNetStreamOpen(sarg->input, 'r');

  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* get lower and upper tag values */
        ltag_val = SNetRecGetTag( rec, sarg->ltag);
        utag_val = SNetRecGetTag( rec, sarg->utag);

        /* for all tag values */
        for( i = ltag_val; i <= utag_val; i++) {
          snet_stream_desc_t *outstream = HashtabGet( repos_tab, i);

          if( outstream == NULL) {
            snet_stream_t *temp_stream;
            snet_stream_t *newstream_addr = SNetStreamCreate(0);
            /* instance does not exist yet, create it */
            outstream = SNetStreamOpen(newstream_addr, 'w');
            /* add to lookup table */
            HashtabPut( repos_tab, i, outstream);
            /* add to list */
            SNetStreamsetPut( &repos_set, outstream);

            /* create info and location vector for creation of this replica */
            info = SNetInfoCopy(sarg->info);
            locvec = SNetLocvecSplitSpawn(SNetLocvecGet(sarg->info), i);
            SNetLocvecSet(info, locvec);

            if( sarg->is_byloc) {
              SNetRouteDynamicEnter(info, i, i, sarg->boxfun);
              temp_stream = sarg->boxfun(newstream_addr, info, i);
              temp_stream = SNetRouteUpdate(info, temp_stream, sarg->location);
              SNetRouteDynamicExit(info, i, i, sarg->boxfun);
            } else {
              SNetRouteDynamicEnter(info, i, sarg->location, sarg->boxfun);
              temp_stream = sarg->boxfun(newstream_addr, info, sarg->location);
              temp_stream = SNetRouteUpdate(info, temp_stream, sarg->location);
              SNetRouteDynamicExit(info, i, sarg->location, sarg->boxfun);
            }

            /* destroy info and location vector */
            SNetLocvecDestroy(locvec);
            SNetInfoDestroy(info);

            if(temp_stream != NULL) {
              /* notify collector about the new instance via initial */
              SNetStreamWrite( initial,
                  SNetRecCreate( REC_collect, temp_stream));
            }
          } /* end if (outstream==NULL) */

          /* multicast the record */
          SNetStreamWrite( outstream,
              /* copy record for all but the last tag value */
              (i!=utag_val) ? SNetRecCopy( rec) : rec
              );
        } /* end for all tags  ltag_val <= i <= utag_val */

        /* If deterministic, append a sort record to *all* registered
         * instances and the initial stream.
         */
        if( sarg->is_det ) {
          /* reset iterator */
          SNetStreamIterReset( iter, &repos_set);
          while( SNetStreamIterHasNext( iter)) {
            snet_stream_desc_t *cur_stream = SNetStreamIterNext( iter);

            SNetStreamWrite( cur_stream,
                SNetRecCreate( REC_sort_end, 0, counter));
          }
          /* Now also send a sort record to initial,
             after the collect records for new instances have been sent */
          SNetStreamWrite( initial,
              SNetRecCreate( REC_sort_end, 0, counter));
        }
        /* increment counter for deterministic variant */
        counter += 1;
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream( rec);
          SNetStreamReplace( instream, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_sort_end:
        /* broadcast the sort record */
        SNetStreamIterReset( iter, &repos_set);
        /* all instances receive copies of the record */
        while( SNetStreamIterHasNext( iter)) {
          snet_stream_desc_t *cur_stream = SNetStreamIterNext( iter);
          SNetStreamWrite( cur_stream,
              SNetRecCreate( REC_sort_end,
                /* we have to increase level */
                SNetRecGetLevel( rec)+1,
                SNetRecGetNum( rec))
              );
        }
        /* send the original record to the initial stream,
           but with increased level */
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetStreamWrite( initial, rec);
        break;

      case REC_terminate:

        SNetStreamIterReset( iter, &repos_set);
        /* all instances receive copies of the record */
        while( SNetStreamIterHasNext( iter)) {
          snet_stream_desc_t *cur_stream = SNetStreamIterNext( iter);
          SNetStreamWrite( cur_stream, SNetRecCopy( rec));

          SNetStreamIterRemove( iter);
          /* close  the stream to the instance */
          SNetStreamClose( cur_stream, false);
        }
        /* send the original record to the initial stream */
        SNetStreamWrite( initial, rec);
        /* note that no sort record has to be appended */
        terminate = true;
        break;

      case REC_collect:
        /* invalid control record */
      default:
        assert( 0);
        /* if ignore, at least destroy it */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  /* destroy repository */
  HashtabDestroy( repos_tab);
  SNetStreamIterDestroy( iter);

  /* close and destroy initial stream */
  SNetStreamClose( initial, false);
  /* close instream */
  SNetStreamClose( instream, true);

  SNetLocvecDestroy(SNetLocvecGet(sarg->info));
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
  snet_info_t *newInfo = SNetInfoCopy(info);
  snet_stream_t *initial, *output;
  split_arg_t *sarg;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  SNetLocvecSplitEnter(locvec);
  SNetLocvecSet(newInfo, SNetLocvecCopy(locvec));

  input = SNetRouteUpdate(newInfo, input, location);
  if(SNetDistribIsNodeLocation(location)) {
    initial = SNetStreamCreate(0);

    sarg = (split_arg_t *) SNetMemAlloc( sizeof( split_arg_t));
    sarg->input  = input;
    sarg->output = initial;
    sarg->boxfun = box_a;
    sarg->info = newInfo;
    sarg->ltag = ltag;
    sarg->utag = utag;
    sarg->is_det = is_det;
    sarg->is_byloc = is_byloc;
    sarg->location = location;
    SNetThreadingSpawn(
        SNetEntityCreate( ENTITY_split, location, locvec,
          "<split>", SplitBoxTask, (void*)sarg)
        );

    output = CollectorCreateDynamic( initial, location, info);

  } else {
    SNetLocvecDestroy(SNetLocvecGet(newInfo));
    SNetInfoDestroy(newInfo);
    output = input;
  }
  SNetLocvecSplitLeave(locvec);

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
