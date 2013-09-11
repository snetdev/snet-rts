/**************************************************************************
 *
 * Implement an optimization for the combination of star and synchro-cell.
 *
 *****************************************************************************/

#include "node.h"

/* test a single bit in mask */
#define HAS_BIT(mask,bit)       ((mask) & (1 << (bit)))
/* set a single bit in mask */
#define SET_BIT(mask,bit)       ((mask) |= (1 << (bit)))
/* clear a single bit in mask */
#define CLR_BIT(mask,bit)       ((mask) &= ~(1 << (bit)))
/* create a new mask of given width with all bits set */
#define NEW_MASK(width)         ((1UL << (width)) - 1UL)

typedef unsigned long mask_t;   /* at most 64 bits wide synchro-cells */

/* a matching is a synchro-cell in progress */
struct matching {
  matching_t     *next;         /* pointer to next matching in linked list */
  matching_t     *same;         /* pointer in list with same unmatched value */
  matching_t     *tail;         /* tail pointer in same list */
  mask_t          unmatched;    /* mask of patterns which are still sought for */
  snet_record_t  *storage[0];   /* array of pointers to records */
};

/* Create a new matching */
static matching_t *NewMatching(unsigned int sync_width)
{
  matching_t *match = (matching_t *)SNetMemAlloc(sizeof(matching_t) +
                                    sync_width * sizeof(snet_record_t *) +
                                    LINE_SIZE);
  match->next = NULL;
  match->same = NULL;
  match->tail = NULL;
  match->unmatched = NEW_MASK(sync_width);
  return match;
}

/* Test if a record matches one of several exit conditions */
static bool MatchExitPatterns(snet_record_t *rec, const zipper_arg_t *zarg)
{
  int i;
  snet_variant_t *pattern;

  LIST_ENUMERATE(zarg->exit_patterns, i, pattern) {
    if (SNetRecPatternMatches(pattern, rec) &&
        SNetEevaluateBool( SNetExprListGet( zarg->exit_guards, i), rec)) {
      return true;
    }
  }

  return false;
}

/*
 * Merges the other records from the storage into the first pattern record.
 * After that, all (but the first) records are destroyed from the storage.
 */
static snet_record_t *MergeFromStorage(
  const zipper_arg_t *zarg,
  matching_t *match,
  snet_record_t *rec)
{
  int                    i, name, value;
  snet_ref_t            *field;
  snet_variant_t        *pattern;
  snet_record_t         *result = match->storage[0];
  void                  *detref = DATA_REC(rec, detref);

  DATA_REC(rec, detref) = NULL;
  match->storage[0] = NULL;
  LIST_ENUMERATE(zarg->sync_patterns, i, pattern) {
    if (match->storage[i] != NULL) {

      RECORD_FOR_EACH_FIELD(match->storage[i], name, field) {
        if (SNetVariantHasField(pattern, name)) {
            SNetRecSetField(result, name, SNetRefCopy(field));
        }
      }

      RECORD_FOR_EACH_TAG(match->storage[i], name, value) {
        if (SNetVariantHasTag(pattern, name)) {
            SNetRecSetTag(result, name, value);
        }
      }

      /* free storage */
      SNetRecDestroy(match->storage[i]);
      match->storage[i] = NULL;
    }
  }
  match->storage[0] = NULL;

  DATA_REC(result, detref) = detref;

  return result;
}

/* Merge a data record */
void SNetNodeZipperData(
    snet_record_t *rec,
    const zipper_arg_t *zarg,
    landing_zipper_t *land)
{
  snet_variant_t *pattern;
  snet_expr_t    *expr;
  matching_t     *match = land->head;
  matching_t     *prev = NULL;
  mask_t          untested = NEW_MASK(zarg->sync_width);
  int             new_matches = 0;
  int             i;
  
  if (match == NULL) {
    match = land->head = NewMatching(zarg->sync_width);
  }
  while (untested) {
    if ((untested & match->unmatched) != 0) {
      LIST_ZIP_ENUMERATE(zarg->sync_patterns, zarg->sync_guards, i, pattern, expr) {
        if (HAS_BIT(untested & match->unmatched, i)) {
          CLR_BIT(untested, i);
          if (SNetRecPatternMatches(pattern, rec) && SNetEevaluateBool(expr, rec)) {
            match->storage[i] = (++new_matches == 1) ? rec : NULL;
            if (CLR_BIT(match->unmatched, i) == 0) {
              break;
            }
          }
        }
      }
      if (new_matches) {
        if (match->same) {
          assert(match->same->next == NULL);
          match->same->next = match->next;
          match->next = match->same;
          assert(match->same->tail == NULL);
          match->same->tail = match->tail;
          match->same = match->tail = NULL;
        }
        if (match->unmatched) {
          SNetRecDetrefDestroy(rec, &land->outdesc);
          if (prev && prev->unmatched == match->unmatched) {
            prev->next = match->next;
            match->next = NULL;
            if (prev->tail) {
              prev->tail->same = match;
              prev->tail = match;
              assert(match->same == NULL);
              assert(match->tail == NULL);
            } else {
              assert(prev->same == NULL);
              prev->same = prev->tail = match;
            }
          }
          return;
        }
        rec = MergeFromStorage(zarg, match, rec);
        if (prev) {
          prev->next = match->next;
        } else {
          land->head = match->next;
        }
        SNetDelete(match);
        if (MatchExitPatterns(rec, zarg)) {
          SNetWrite(&land->outdesc, rec, true);
          return;
        }
        match = prev;
        untested = NEW_MASK(zarg->sync_width);
        new_matches = 0;
      }
    }
    if ((prev = match) == NULL) {
      assert(land->head == NULL);
      match = land->head = NewMatching(zarg->sync_width);
    }
    else if ((match = match->next) == NULL) {
      match = prev->next = NewMatching(zarg->sync_width);
      assert(!prev || prev->unmatched != match->unmatched);
    }
  }
  SNetUtilDebugNoticeEnt(zarg->entity,
      "[MERGE]: record doesn't match sync or exit pattern\n");
  SNetRecDestroy(rec);
}

/* Destroy the landing state of a Zipper node */
void SNetNodeZipperTerminate(landing_zipper_t *land, zipper_arg_t *zarg)
{
  matching_t            *match, *same, *prev = NULL;
  int                    i;
  unsigned int           count = 0;

  while ((match = land->head) != NULL) {
    land->head = match->next;
    do {
      same = match->same;
      for (i = 0; i < zarg->sync_width; ++i) {
        if (!HAS_BIT(match->unmatched, i) && match->storage[i] != NULL) {
          if (prev != match) {
            prev = match;
            ++count;
          }
          if (SNetVerbose()) {
            char buf[200];
            SNetRecordTypeString(match->storage[i], buf, sizeof buf);
            fprintf(stderr, "\tSync-cell %2d, Record %2d: %s\n", count, i, buf);
          }
          SNetRecDestroy(match->storage[i]);
        }
      }
      SNetDelete(match);
    } while ((match = same) != NULL);
  }
  if (count) {
    SNetUtilDebugNoticeEnt(zarg->entity, "[MERGE] Warning: "
      "Destroying %u partially synchronized sync-cells!", count);
  }
}

/* Process records for fused sync-star. */
void SNetNodeZipper(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const zipper_arg_t     *zarg = DESC_NODE_SPEC(desc, zipper);
  landing_zipper_t       *land = DESC_LAND_SPEC(desc, zipper);

  trace(__func__);

  if (land->outdesc == NULL) {
    land->outdesc = SNetStreamOpen(zarg->output, desc);
  }
  switch (REC_DESCR(rec)) {
    case REC_data:
      if (MatchExitPatterns(rec, zarg)) {
        SNetWrite(&land->outdesc, rec, true);
      } else {
        SNetNodeZipperData(rec, zarg, land);
      }
      break;

    case REC_detref:
      /* forward record */
      SNetWrite(&land->outdesc, rec, true);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, zarg->entity);
  }
}

/* Terminate a zipper landing. */
void SNetTermZipper(landing_t *land, fifo_t *fifo)
{
  landing_zipper_t      *lzip = LAND_SPEC(land, zipper);
  zipper_arg_t          *zarg = NODE_SPEC(land->node, zipper);

  trace(__func__);

  SNetNodeZipperTerminate(lzip, zarg);
  if (lzip->outdesc) {
    SNetFifoPut(fifo, lzip->outdesc);
  }
  SNetFreeLanding(land);
}

/* Free a merge node */
void SNetStopZipper(node_t *node, fifo_t *fifo)
{
  zipper_arg_t *zarg = NODE_SPEC(node, zipper);
  trace(__func__);
  SNetStopStream(zarg->output, fifo);
  SNetVariantListDestroy(zarg->exit_patterns);
  SNetExprListDestroy(zarg->exit_guards);
  SNetVariantListDestroy(zarg->sync_patterns);
  SNetExprListDestroy(zarg->sync_guards);
  SNetEntityDestroy(zarg->entity);
  SNetMemFree(node);
}

/* Create a new Zipper */
snet_stream_t *SNetZipper(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t    *exit_guards,
    snet_variant_list_t *sync_patterns,
    snet_expr_list_t    *sync_guards )
{
  snet_stream_t *output;
  node_t        *node;
  zipper_arg_t  *zarg;

  trace(__func__);
  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_zipper, location, &input, 1, &output, 1,
                     SNetNodeZipper, SNetStopZipper, SNetTermZipper);
  zarg                = NODE_SPEC(node, zipper);
  zarg->output        = output;
  zarg->exit_patterns = exit_patterns;
  zarg->exit_guards   = exit_guards;
  zarg->sync_patterns = sync_patterns;
  zarg->sync_guards   = sync_guards;
  zarg->sync_width    = SNetVariantListLength( zarg->sync_patterns);
  zarg->entity = SNetEntityCreate( ENTITY_star, location, SNetLocvecGet(info),
                                   "<syncstar>", NULL, (void *) zarg);
  if (zarg->sync_width > 8*sizeof(mask_t)) {
    SNetUtilDebugFatalEnt(zarg->entity,
                          "[%s]: number of patterns %u exceeds mask width %zu\n",
                          __func__, zarg->sync_width, 8*sizeof(mask_t));
  }

  return output;
}

