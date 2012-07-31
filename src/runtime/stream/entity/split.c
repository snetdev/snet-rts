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
  snet_stream_desc_t *initial, *instream;
  snet_startup_fun_t boxfun;
  snet_info_t *info;
  int ltag, utag;
  bool is_det;
  bool is_byloc;
  int location;
  /* a list of all outstreams for all yet created instances */
  snet_streamset_t repos_set;
  snet_stream_iter_t *iter;
  /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
  hashtab_t *repos_tab;
  /* for deterministic variant: */
  int counter;
} split_arg_t;

/**
 * Split box task.
 *
 * Implements both the non-deterministic and deterministic variants.
 */
static void SplitBoxTask(void *arg)
{
  split_arg_t *sarg = arg;
  int i;

  int ltag_val, utag_val;
  snet_info_t *info;
  snet_record_t *rec;
  snet_locvec_t *locvec;




  /* read from input stream */
  rec = SNetStreamRead( sarg->instream);

  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* get lower and upper tag values */
      ltag_val = SNetRecGetTag( rec, sarg->ltag);
      utag_val = SNetRecGetTag( rec, sarg->utag);

      /* for all tag values */
      for( i = ltag_val; i <= utag_val; i++) {
        snet_stream_desc_t *outstream = HashtabGet( sarg->repos_tab, i);

        if( outstream == NULL) {
          snet_stream_t *temp_stream;
          snet_stream_t *newstream_addr = SNetStreamCreate(0);
          /* instance does not exist yet, create it */
          outstream = SNetStreamOpen(newstream_addr, 'w');
          /* add to lookup table */
          HashtabPut( sarg->repos_tab, i, outstream);
          /* add to list */
          SNetStreamsetPut( &sarg->repos_set, outstream);

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
            SNetStreamWrite( sarg->initial,
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
        SNetStreamIterReset( sarg->iter, &sarg->repos_set);
        while( SNetStreamIterHasNext( sarg->iter)) {
          snet_stream_desc_t *tmp = SNetStreamIterNext( sarg->iter);
          snet_stream_desc_t *cur_stream;
          cur_stream = SNetStreamOpen(SNetStreamGet(tmp), 'w');

          SNetStreamWrite( cur_stream,
              SNetRecCreate( REC_sort_end, 0, sarg->counter));
          SNetStreamClose( cur_stream, false);
        }
        /* Now also send a sort record to initial,
           after the collect records for new instances have been sent */
        SNetStreamWrite( sarg->initial,
            SNetRecCreate( REC_sort_end, 0, sarg->counter));
      }
      /* increment counter for deterministic variant */
      sarg->counter += 1;
      break;

    case REC_sync:
      SNetStreamReplace( sarg->instream, SNetRecGetStream( rec));
      SNetRecDestroy( rec);
      break;

    case REC_sort_end:
      /* broadcast the sort record */
      SNetStreamIterReset( sarg->iter, &sarg->repos_set);
      /* all instances receive copies of the record */
      while( SNetStreamIterHasNext( sarg->iter)) {
        snet_stream_desc_t *tmp = SNetStreamIterNext( sarg->iter);
        snet_stream_desc_t *cur_stream;
        cur_stream = SNetStreamOpen(SNetStreamGet(tmp), 'w');
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
      SNetStreamWrite( sarg->initial, rec);
      break;

    case REC_terminate:

      SNetStreamIterReset( sarg->iter, &sarg->repos_set);
      /* all instances receive copies of the record */
      while( SNetStreamIterHasNext( sarg->iter)) {
        snet_stream_desc_t *cur_stream = SNetStreamIterNext( sarg->iter);
        SNetStreamWrite( cur_stream, SNetRecCopy( rec));

        SNetStreamIterRemove( sarg->iter);
        /* close  the stream to the instance */
        SNetStreamClose( cur_stream, false);
      }
      /* send the original record to the initial stream */
      SNetStreamWrite( sarg->initial, rec);
      /* note that no sort record has to be appended */


      /* destroy repository */
      HashtabDestroy( sarg->repos_tab);
      SNetStreamIterDestroy( sarg->iter);

      /* close and destroy initial stream */
      SNetStreamClose( sarg->initial, false);
      /* close instream */
      SNetStreamClose( sarg->instream, true);

      SNetLocvecDestroy(SNetLocvecGet(sarg->info));
      SNetInfoDestroy(sarg->info);
      /* destroy the argument */
      SNetMemFree( sarg);
      return;

    case REC_collect:
      /* invalid control record */
    default:
      assert( 0);
      /* if ignore, at least destroy it */
      SNetRecDestroy( rec);
  }
  SNetThreadingRespawn(NULL);
}


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

    sarg = SNetMemAlloc( sizeof( split_arg_t));
    sarg->instream  = SNetStreamOpen(input, 'r');
    sarg->initial = SNetStreamOpen(initial, 'w');
    sarg->boxfun = box_a;
    sarg->info = newInfo;
    sarg->ltag = ltag;
    sarg->utag = utag;
    sarg->is_det = is_det;
    sarg->is_byloc = is_byloc;
    sarg->location = location;
    /* a list of all outstreams for all yet created instances */
    sarg->repos_set = NULL;
    sarg->iter = SNetStreamIterCreate( &sarg->repos_set);
    /* a hashtable for fast lookup, initial capacity = 2^4 = 16 */
    sarg->repos_tab = HashtabCreate( 4);

    sarg->counter = 0;
    SNetThreadingSpawn( ENTITY_split, location, locvec,
          "<split>", &SplitBoxTask, sarg);

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
