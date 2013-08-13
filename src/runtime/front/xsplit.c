#include "node.h"

enum { WriteCollector = -1 };

/* Write a record to a stream which may have to be opened first. */
static void SplitWrite(
    int idx,
    snet_record_t *rec,
    snet_stream_desc_t *desc,
    bool last)
{
  const split_arg_t     *sarg = DESC_NODE_SPEC(desc, split);
  landing_split_t       *land = DESC_LAND_SPEC(desc, split);
  snet_stream_desc_t    *instdesc;

  if (idx == WriteCollector) {
    if (land->colldesc == NULL) {
      if (SNetTopLanding(desc) != land->collland) {
        SNetPushLanding(desc, land->collland);
      }
      land->colldesc = SNetStreamOpen(sarg->collector, desc);
    }
    SNetWrite(&land->colldesc, rec, last);
  } else {
    /* Write to an instance: look for an existing connection. */
    void **inst_ptr = HashtabGetPointer( land->hashtab, idx);
    if (inst_ptr == NULL) {
      /* Make sure the collector landing is on top. */
      if (SNetTopLanding(desc) != land->collland) {
        SNetPushLanding(desc, land->collland);
      }
      /* Check for indexed placement. */
      if (sarg->is_byloc) {
        /* Communicate destination location to subsequent landings. */
        desc->landing->dyn_locs[DESC_NODE(desc)->loc_split_level - 1] = idx;
      }
      /* Connect via a stream to the subnetwork. */
      instdesc = SNetStreamOpen( sarg->instance, desc);
      /* Remember the connection for future use. */
      HashtabPut( land->hashtab, idx, instdesc);
      /* Count the number of outgoing connections. */
      land->split_count++;
      /* This is tricky because we pass a pointer to a local variable,
       * but the first write after open cannot be garbage collected. */
      SNetWrite(&instdesc, rec, last);
    } else {
      /* Reuse an existing connection. */
      SNetWrite((snet_stream_desc_t **) inst_ptr, rec, last);
    }
  }
}

/* Split node process a record. */
void SNetNodeSplit(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const split_arg_t     *sarg = DESC_NODE_SPEC(desc, split);
  int                    i, ltag_val, utag_val;

  trace(__func__);

  switch (REC_DESCR(rec)) {
    case REC_data:
      if (sarg->is_det | sarg->is_detsup) {
        landing_split_t *land = DESC_LAND_SPEC(desc, split);
        SNetDetEnter(rec, &land->detenter, sarg->is_det, sarg->entity);
      }

      /* get lower and upper tag values */
      ltag_val = SNetRecGetTag( rec, sarg->ltag);
      utag_val = SNetRecGetTag( rec, sarg->utag);

      /* for all tag values */
      for (i = ltag_val; i <= utag_val; i++) {
        /* copy record for all but the last tag value */
        bool last = (i == utag_val);
        SplitWrite( i, last ? rec : SNetRecCopy(rec), desc, last);
      }
      break;

    case REC_detref:
      /* forward record */
      SplitWrite(WriteCollector, rec, desc, true);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, sarg->entity);
  }
}

/* Terminate a split landing. */
void SNetTermSplit(landing_t *land, fifo_t *fifo)
{
  landing_split_t       *lsplit = LAND_SPEC(land, split);
  hashtab_iter_t        *iter;
  snet_stream_desc_t    *desc;

  trace(__func__);

  if (lsplit->split_count > 0) {
    /* Loop over all open instances */
    iter = HashtabIterCreate(lsplit->hashtab);
    while ((desc = HashtabIterNext(iter)) != NULL) {
      SNetFifoPut(fifo, desc);
    }
    HashtabIterDestroy(iter);

    if (lsplit->colldesc) {
      SNetFifoPut(fifo, lsplit->colldesc);
    }
  }

  SNetLandingDone(lsplit->collland);
  SNetFreeLanding(land);
}

/* Destroy a split node. */
void SNetStopSplit(node_t *node, fifo_t *fifo)
{
  split_arg_t *sarg = NODE_SPEC(node, split);
  trace(__func__);

  SNetStopStream(sarg->instance, fifo);
  SNetStopStream(sarg->collector, fifo);
  SNetEntityDestroy(sarg->entity);
  SNetDelete(node);
}


/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/

/* Keep track of the nesting level of indexed placement combinators. */
static int snet_loc_split_level;

/* Return the current indexed placement stack level. */
int SNetLocSplitGetLevel(void)
{
  trace(__func__);
  return snet_loc_split_level;
}

/* Increment the indexed placement stack level by one. */
void SNetLocSplitIncrLevel(void)
{
  trace(__func__);
  ++snet_loc_split_level;
}

/* Decrement the indexed placement stack level by one. */
void SNetLocSplitDecrLevel(void)
{
  trace(__func__);
  --snet_loc_split_level;
  assert(snet_loc_split_level >= 0);
}

/* Keep track of the nesting level of combinator subnetworks. */
static int snet_subnet_level;

/* Return the current indexed placement stack level. */
int SNetSubnetGetLevel(void)
{
  trace(__func__);
  return snet_subnet_level;
}

/* Increment the indexed placement stack level by one. */
void SNetSubnetIncrLevel(void)
{
  trace(__func__);
  ++snet_subnet_level;
}

/* Decrement the indexed placement stack level by one. */
void SNetSubnetDecrLevel(void)
{
  trace(__func__);
  --snet_subnet_level;
  assert(snet_subnet_level >= 0);
}

/**
 * Convenience function for creating a Split, DetSplit, LocSplit or LocSplitDet.
 * This is determined by the parameters 'is_byloc' and 'is_det'.
 * Parameter 'is_byloc' says whether to create an indexed placement combinator.
 * Parameter 'is_det' specifies if the combinator is deterministic.
 */
snet_stream_t *CreateSplit(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag,
    bool is_byloc,
    bool is_det)
{
  snet_stream_t *collector;
  snet_stream_t *output;
  snet_stream_t *instout;
  node_t        *node;
  split_arg_t   *sarg;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  SNetLocvecSplitEnter(locvec);

  /* keep track of entering indexed placement. */
  if (is_byloc) {
    SNetLocSplitIncrLevel();
  }

  collector = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_split, location, &input, 1, &collector, 1,
                     SNetNodeSplit, SNetStopSplit, SNetTermSplit);
  sarg = NODE_SPEC(node, split);
  sarg->collector = collector;
  sarg->ltag = ltag;
  sarg->utag = utag;
  sarg->is_det = is_det;
  sarg->is_detsup = (SNetDetGetLevel() > 0);
  sarg->is_byloc = is_byloc;
  sarg->entity = SNetEntityCreate( ENTITY_split, location, locvec,
                                   "<split>", NULL, (void*)sarg);

  /* create collector */
  output = SNetCollectorDynamic(collector, location, info, is_det, node);

  /* create replica */
  sarg->instance = SNetNodeStreamCreate(node);
  SNetSubnetIncrLevel();
  instout = (*box_a)(sarg->instance, info, LOCATION_UNKNOWN);
  SNetSubnetDecrLevel();
  SNetCollectorAddStream(STREAM_DEST(sarg->collector), instout);

  /* keep track of leaving indexed placement. */
  if (is_byloc) {
    SNetLocSplitDecrLevel();
  }

  SNetLocvecSplitLeave(locvec);

  return output;
}

/* Non-det Split creation function */
snet_stream_t *SNetSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  trace(__func__);
  return CreateSplit( input, info, location, box_a, ltag, utag,
                      false, /* not by location */
                      false);  /* not det */
}

/* Det Split creation function */
snet_stream_t *SNetSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  snet_stream_t *outstream;
  trace(__func__);
  SNetDetIncrLevel();
  outstream = CreateSplit( input, info, location, box_a, ltag, utag,
                           false, /* not by location */
                           true);   /* is det */
  SNetDetDecrLevel();
  return outstream;
}

/* Non-det Location Split creation function */
snet_stream_t *SNetLocSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  trace(__func__);
  return CreateSplit( input, info, location, box_a, ltag, utag,
                      SNetDistribIsDistributed(), /* is by location */
                      false); /* not det */
}

/* Det Location Split creation function */
snet_stream_t *SNetLocSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag)
{
  snet_stream_t *outstream;
  trace(__func__);
  SNetDetIncrLevel();
  outstream = CreateSplit( input, info, location, box_a, ltag, utag,
                           SNetDistribIsDistributed(), /* is by location */
                           true);  /* is det */
  SNetDetDecrLevel();
  return outstream;
}

