#include "node.h"
#include <string.h>

static inline void MatchCountUpdate( match_count_t *mc, bool match_cond)
{
  if (match_cond) {
    mc->count += 1;
  } else {
    mc->is_match = false;
  }
}


static void CheckMatch(
    snet_record_t *rec,
    snet_variant_list_t *variant_list,
    match_count_t *mc,
    bool terminated)
{
  int name, val;
  snet_variant_t *variant;
  int max = -1;

  assert(rec != NULL);
  assert(variant_list != NULL);
  assert(mc != NULL);

  if (terminated == false) {
    /* for all variants */
    LIST_FOR_EACH(variant_list, variant) {
      mc->count = 0;
      mc->is_match = true;

      VARIANT_FOR_EACH_FIELD(variant, name) {
        MatchCountUpdate(mc, SNetRecHasField(rec, name));
      }

      VARIANT_FOR_EACH_TAG(variant, name) {
        MatchCountUpdate(mc, SNetRecHasTag(rec, name));
      }

      RECORD_FOR_EACH_BTAG(rec, name, val) {
        MatchCountUpdate(mc, SNetVariantHasBTag(variant, name));
        (void) val; /* prevent compiler warnings */
      }

      if (mc->is_match && mc->count > max) {
        max = mc->count;
      }
    }
  }

  if (max >= 0) {
    mc->is_match = true;
    mc->count = max;
  } else {
    mc->is_match = false;
  }
}


static bool VariantIsSupertypeOfAllOthers(
    snet_variant_t *var,
    snet_variant_list_t *variant_list)
{
  snet_variant_t *other;
  int name;

  /* for all variants in the list */
  LIST_FOR_EACH(variant_list, other) {
    VARIANT_FOR_EACH_FIELD(var, name) {
      if (!SNetVariantHasField(other, name)) return false;
    }

    VARIANT_FOR_EACH_TAG(var, name) {
      if (!SNetVariantHasTag(other, name)) return false;
    }

    VARIANT_FOR_EACH_BTAG(var, name) {
      if (!SNetVariantHasBTag(other, name)) return false;
    }
  }
  return true;
}


/**
 * Check for "best match" and decide which buffer to dispatch to
 * in case of a draw.
 */
static int BestMatch( const match_count_t *counter, int num)
{
  int i;
  int res, max;

  res = -1;
  max = -1;
  for (i = 0; i < num; i++) {
    if (counter[i].is_match) {
      if (counter[i].count > max) {
        res = i;
        max = counter[i].count;
      }
    }
  }
  return res;
}

enum { WriteCollector = -1 };

static void ParallelOpen(int idx, snet_stream_desc_t *desc)
{
  const parallel_arg_t  *parg = DESC_NODE_SPEC(desc, parallel);
  landing_parallel_t    *land = DESC_LAND_SPEC(desc, parallel);

  if (SNetTopLanding(desc) != land->collland) {
    SNetPushLanding(desc, land->collland);
  }
  if (idx == WriteCollector) {
    assert(land->colldesc == NULL);
    land->colldesc = SNetStreamOpen(parg->collector, desc);
  } else {
    assert(land->outdescs[idx] == NULL);
    if (land->terminated && land->terminated[idx]) {
      SNetUtilDebugFatalEnt(parg->entity,
                            "[%s]: unexpected record on a terminated branch.",
                            __func__);
    }
    land->outdescs[idx] = SNetStreamOpen(parg->outputs[idx], desc);
  }
}

static void ParallelWrite(int idx, snet_record_t *rec, snet_stream_desc_t *desc)
{
  landing_parallel_t    *land   = DESC_LAND_SPEC(desc, parallel);

  if (idx == WriteCollector) {
    if (land->colldesc == NULL) {
      ParallelOpen(idx, desc);
    }
    SNetWrite(&land->colldesc, rec, true);
  } else {
    if (land->outdescs[idx] == NULL) {
      ParallelOpen(idx, desc);
    }
    SNetWrite(&land->outdescs[idx], rec, true);
  }
}

/* Turn a parallel landing into an identity landing. */
static void ParallelBecomeIdentity(int last, snet_stream_desc_t *desc)
{
  landing_parallel_t    *land   = DESC_LAND_SPEC(desc, parallel);

  if (land->outdescs[last] == NULL) {
    ParallelOpen(last, desc);
  }
  if (land->colldesc) {
    SNetDescDone(land->colldesc);
  }
  SNetLandingDone(land->collland);
  SNetBecomeIdentity(desc->landing, land->outdescs[last]);
}

/* Process a sync record. */
static void ParallelReceiveSync(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const parallel_arg_t  *parg   = DESC_NODE_SPEC(desc, parallel);
  landing_parallel_t    *land   = DESC_LAND_SPEC(desc, parallel);
  snet_variant_t        *sync_type;
  int                    i, count = 0, last = 0;

  trace(__func__);

  /* Extract type variant from sync record. */
  sync_type = SNetRecGetVariant(rec);
  SYNC_REC( rec, outtype) = NULL;
  SNetRecDestroy(rec);
  if (sync_type == NULL) {
    return;
  }

  /* Keep track of which branches are being terminated by the sync record */
  if (land->terminated == NULL) {
    land->terminated = SNetNewN(parg->num, bool);
    for (i = 0; i < parg->num; ++i) {
      land->terminated[i] = false;
    }
  }

  /* Close branches which have become unreachable. */
  for (i = 0; i < parg->num; ++i) {
    if (land->terminated[i] == false) {
      snet_variant_list_t *vl = SNetVariantListListGet(parg->variant_lists, i);
      if (VariantIsSupertypeOfAllOthers(sync_type, vl)) {
        /* char buf[200], vls[200];
        SNetVariantString(sync_type, buf, sizeof buf);
        SNetVariantListString(vl, vls, sizeof vls);
        printf("par sup %s %s\n", buf, vls); */
        if (land->outdescs[i]) {
          SNetDescDone(land->outdescs[i]);
          land->outdescs[i] = NULL;
        }
        land->terminated[i] = true;
      } else {
        /* char buf[200], vls[200];
        SNetVariantString(sync_type, buf, sizeof buf);
        SNetVariantListString(vl, vls, sizeof vls);
        printf("par not %s %s\n", buf, vls); */
        ++count;
        last = i;
      }
    }
  }
  SNetVariantDestroy(sync_type);

  /* if only one branch left, then schedule for garbage collection. */
  if (count == 1) {
    ParallelBecomeIdentity(last, desc);
  }
}

/* Initialize a parallel landing. */
static bool ParallelInit(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const parallel_arg_t  *parg   = DESC_NODE_SPEC(desc, parallel);
  landing_parallel_t    *land   = DESC_LAND_SPEC(desc, parallel);
  snet_variant_list_t   *variants;
  int                    i;
  int                    num_init_branches = 0;
  int                    last = 0;

  trace(__func__);

  /* Keep track of which branches have been terminated. */
  land->terminated = SNetNewN(parg->num, bool);
  for (i = 0; i < parg->num; ++i) {
    land->terminated[i] = false;
  }

  /* Test for initialiser branches */
  for (i = 0; i < parg->num; i++) {
    variants = SNetVariantListListGet( parg->variant_lists, i);
    if (SNetVariantListLength( variants) == 0) {
      snet_record_t *trigger = SNetRecCreate( REC_trigger_initialiser);
      ParallelOpen(i, desc);
      SNetStreamWrite(land->outdescs[i], trigger);
      SNetDescDone(land->outdescs[i]);
      land->outdescs[i] = NULL;
      land->terminated[i] = true;
      ++num_init_branches;
    } else {
      last = i;
    }
  }

  land->initialized = true;

  if (parg->num == num_init_branches) {
    /* Nothing left to do when no more outputs remain. */
    SNetRecDestroy(rec);
    return false;
  }
  if (parg->num == 1 + num_init_branches) {
    /* No need for a dispatcher when only one output remains. */
    ParallelWrite(last, rec, desc);
    ParallelBecomeIdentity(last, desc);
    return false;
  }
  return true;
}

/* Report in detail on a routing problem. */
static void RoutingError(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const parallel_arg_t  *parg   = DESC_NODE_SPEC(desc, parallel);
  snet_variant_list_t   *variants;
  int                    i;
  size_t                 len = 0;
  char                   recbuf[200];
  char                   report[2000];

  SNetRecordTypeString(rec, recbuf, sizeof recbuf);
  report[0] = '\0';
  LIST_ENUMERATE(parg->variant_lists, i, variants) {
    if (len + 100 >= sizeof(report)) {
      break;
    }
    sprintf(report + len, "\tVariant %d: ", i + 1);
    len += strlen(report + len);
    SNetVariantListString(variants, report + len, sizeof(report) - len);
    len += strlen(report + len);
    if (len + 2 < sizeof(report)) {
      strcpy(report + len, "\n");
      len += strlen(report + len);
    }
  }
  SNetUtilDebugFatalEnt(parg->entity,
      "[PAR] Cannot route data record %s: no matching branch!\n%s", recbuf, report);
}

/* Process a record for a parallel node. */
void SNetNodeParallel(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const parallel_arg_t  *parg   = DESC_NODE_SPEC(desc, parallel);
  landing_parallel_t    *land   = DESC_LAND_SPEC(desc, parallel);
  snet_variant_list_t   *variants;
  int                    i;

  trace(__func__);

  if (land->initialized == false) {
    if (ParallelInit(desc, rec) == false) {
      return;
    }
  }

  switch (REC_DESCR(rec)) {
    case REC_data:
      if (parg->is_det | parg->is_detsup) {
        SNetDetEnter(rec, &land->detenter, parg->is_det, parg->entity);
      }
      LIST_ENUMERATE(parg->variant_lists, i, variants) {
        CheckMatch( rec, variants, &land->matchcounter[i],
                    land->terminated && land->terminated[i]);
      }
      i = BestMatch( land->matchcounter, parg->num);
      if (i >= 0) {
        ParallelWrite(i, rec, desc);
      } else {
        RoutingError(desc, rec);
      }
      break;

    case REC_detref:
      /* forward record */
      ParallelWrite(WriteCollector, rec, desc);
      break;

    case REC_sync:
      ParallelReceiveSync(desc, rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, parg->entity);
  }
}

/* Terminate a parallel landing. */
void SNetTermParallel(landing_t *land, fifo_t *fifo)
{
  parallel_arg_t        *parg   = NODE_SPEC(land->node, parallel);
  landing_parallel_t    *lpar   = LAND_SPEC(land, parallel);
  int                    i;

  trace(__func__);

  if (lpar->colldesc) {
    SNetFifoPut(fifo, lpar->colldesc);
  }

  for (i = 0; i < parg->num; i++) {
    if (lpar->outdescs[i]) {
      SNetFifoPut(fifo, lpar->outdescs[i]);
    }
  }

  SNetLandingDone(lpar->collland);
  SNetFreeLanding(land);
}

/* Destroy a parallel node. */
void SNetStopParallel(node_t *node, fifo_t *fifo)
{
  parallel_arg_t *parg = NODE_SPEC(node, parallel);
  int i;
  trace(__func__);
  for (i = 0; i < parg->num; ++i) {
    SNetStopStream(parg->outputs[i], fifo);
  }
  SNetStopStream(parg->collector, fifo);
  SNetVariantListListDestroy(parg->variant_lists);
  SNetDeleteN(parg->num, parg->outputs);
  SNetEntityDestroy(parg->entity);
  SNetDelete(node);
}

static snet_stream_t *CreateParallel(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    snet_startup_fun_t *funs,
    bool is_det)
{
  int i;
  node_t         *node;
  parallel_arg_t *parg;
  snet_stream_t  *output;
  snet_stream_t **transits;
  snet_stream_t **collstreams;
  snet_variant_list_t *variants;
  snet_locvec_t  *locvec;
  const int       num = SNetVariantListListLength( variant_lists);

  locvec = SNetLocvecGet(info);
  SNetLocvecParallelEnter(locvec);

  transits    = SNetNewN(num, snet_stream_t*);
  collstreams = SNetNewN(num + 1, snet_stream_t*);

  /* create branches */
  SNetSubnetIncrLevel();
  LIST_ENUMERATE(variant_lists, i, variants) {
    snet_info_t *newInfo = SNetInfoCopy(info);
    transits[i] = SNetStreamCreate(0);
    SNetLocvecParallelNext(locvec);
    collstreams[i] = (*funs[i])(transits[i], newInfo, location);
    SNetInfoDestroy(newInfo);
    (void) variants;    /* prevent compiler warnings */
  }
  SNetSubnetDecrLevel();

  SNetLocvecParallelReset(locvec);

  node = SNetNodeNew(NODE_parallel, location, &input, 1, transits, num,
                     SNetNodeParallel, SNetStopParallel, SNetTermParallel);
  parg = NODE_SPEC(node, parallel);
  parg->outputs = transits;
  parg->variant_lists = variant_lists;
  parg->num = num;
  parg->is_det = is_det;
  parg->is_detsup = (SNetDetGetLevel() > 0);
  parg->entity = SNetEntityCreate( ENTITY_parallel, location, locvec,
                                   "<parallel>", NULL, (void*)parg);

  /* create collector with collstreams */
  parg->collector = collstreams[num] = SNetStreamCreate(0);
  output = SNetCollectorStatic(num + 1, collstreams, location, info,
                               is_det, node);
  SNetDeleteN(num + 1, collstreams);

  SNetLocvecParallelLeave(locvec);
  SNetDeleteN(num, funs);

  return output;
}

snet_stream_t *SNetParallel(
    snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...)
{
  va_list args;
  int i;
  const int num = SNetVariantListListLength( variant_lists);
  snet_startup_fun_t *funs = SNetNewN(num, snet_startup_fun_t);

  trace(__func__);

  va_start( args, variant_lists);
  for (i = 0; i < num; i++) {
    funs[i] = va_arg( args, snet_startup_fun_t);
  }
  va_end( args);

  return CreateParallel( instream, info, location, variant_lists, funs, false);
}

snet_stream_t *SNetParallelDet(
    snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...)
{
  va_list args;
  int i;
  const int num = SNetVariantListListLength( variant_lists);
  snet_startup_fun_t *funs = SNetNewN(num, snet_startup_fun_t);
  snet_stream_t *outstream;

  trace(__func__);

  va_start( args, variant_lists);
  for (i = 0; i < num; i++) {
    funs[i] = va_arg( args, snet_startup_fun_t);
  }
  va_end( args);

  SNetDetIncrLevel();
  outstream = CreateParallel( instream, info, location, variant_lists, funs, true);
  SNetDetDecrLevel();
  return outstream;
}

