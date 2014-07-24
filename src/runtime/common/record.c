#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "interface_functions.h"
#include "distribution.h"
#include "record.h"
#include "memfun.h"
#include "debug.h"
#include "map.h"
#include "variant.h"
#include "atomiccnt.h"

static snet_atomiccnt_t recid_sequencer __attribute__ ((aligned (LINE_SIZE)))
                        = SNET_ATOMICCNT_INITIALIZER(0);

/*****************************************************************************
 * Helper functions
 ****************************************************************************/
static void GenerateRecId(snet_record_id_t *rid)
{
  assert( SNET_REC_SUBID_NUM == 2 );
  rid->subid[0] = SNetAtomicCntFetchAndInc(&recid_sequencer);
  rid->subid[1] = SNetDistribGetNodeId();
}

/* return number of created records */
unsigned SNetGetRecCounter(void)
{
  return recid_sequencer.counter;
}

/*****************************************************************************
 * Compares two record ids
 ****************************************************************************/
bool SNetRecordIdEquals (snet_record_id_t rid1, snet_record_id_t rid2)
{
   bool res = true;
   /* determine array size */
   int i;
   for (i=0; i<SNET_REC_SUBID_NUM; i++) {
       if (rid1.subid[i] != rid2.subid[i]) {
           res = false;
           break;
       }
   }
   return res;
}



bool SNetRecPatternMatches(snet_variant_t *pat, snet_record_t *rec)
{
  int val;

  VARIANT_FOR_EACH_FIELD(pat, val) {
    if (!SNetRecHasField(rec, val)) {
      return false;
    }
  }

  VARIANT_FOR_EACH_TAG(pat, val) {
    if (!SNetRecHasTag(rec, val)) {
      return false;
    }
  }

  VARIANT_FOR_EACH_BTAG(pat, val) {
    if (!SNetRecHasBTag(rec, val)) {
      return false;
    }
  }

  return true;
}


void SNetRecFlowInherit( snet_variant_t *pat, snet_record_t *in_rec,
                             snet_record_t *out_rec)
{
  int name, val;
  snet_ref_t *field;

  RECORD_FOR_EACH_FIELD(in_rec, name, field) {
    if (!SNetVariantHasField( pat, name)) {
      SNetRecSetField( out_rec, name, SNetRefCopy(field));
    }
  }

  RECORD_FOR_EACH_TAG(in_rec, name, val) {
    if (!SNetVariantHasTag( pat, name)) {
      SNetRecSetTag( out_rec, name, val);
    }
  }
}

snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...)
{
  snet_record_t *rec;
  va_list args;

  rec = SNetMemAlloc( sizeof( snet_record_t));
  REC_DESCR( rec) = descr;

  va_start( args, descr);
  switch (descr) {
    case REC_data:
      DATA_REC( rec, btags) = SNetIntMapCreate(0);
      DATA_REC( rec, tags) = SNetIntMapCreate(0);
      DATA_REC( rec, fields) = SNetRefMapCreate(0);
      DATA_REC( rec, mode) = MODE_binary;
      GenerateRecId( &DATA_REC( rec, rid) );
      DATA_REC( rec, interface_id) = 0;
      DATA_REC( rec, detref) = NULL;
      break;
    case REC_trigger_initialiser:
      DATA_REC( rec, detref) = NULL;
      break;
    case REC_sync:
      SYNC_REC( rec, input) = va_arg( args, snet_stream_t *);
      SYNC_REC( rec, outtype) = NULL;
      break;
    case REC_collect:
      COLL_REC( rec, output) = va_arg( args, snet_stream_t*);
      break;
    case REC_terminate:
      TERM_REC( rec, local) = false;
      break;
    case REC_sort_end:
      SORT_E_REC( rec, level) = va_arg( args, int);
      SORT_E_REC( rec, num) = va_arg( args, int);
      break;
    case REC_detref:
      DETREF_REC( rec, seqnr) = va_arg( args, long);
      DETREF_REC( rec, location) = va_arg( args, int);
      DETREF_REC( rec, senderloc) = SNetDistribGetNodeId();
      DETREF_REC( rec, leave) = va_arg( args, void*);
      DETREF_REC( rec, detref) = va_arg( args, void*);
      break;
    case REC_observ:
      OBSERV_REC( rec, oid ) = va_arg( args, int);
      OBSERV_REC( rec, rec ) = va_arg( args, snet_record_t*);
      OBSERV_REC( rec, desc) = va_arg( args, void*);
      break;
    case REC_star_leader:
      LEADER_REC( rec, counter) = va_arg( args, long);
      LEADER_REC( rec, seqnr  ) = va_arg( args, long);
      break;
    case REC_wakeup:
      break;
    default:
      SNetRecUnknown(__func__, rec);
  }
  va_end( args);

  return rec;
}

snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  snet_record_t *new_rec;

  switch (REC_DESCR( rec)) {
    case REC_data:
      new_rec = SNetMemAlloc( sizeof( snet_record_t));
      REC_DESCR( new_rec) = REC_data;
      DATA_REC( new_rec, fields) = SNetRefMapCopy(DATA_REC(rec, fields));
      DATA_REC( new_rec, tags) = SNetIntMapCopy( DATA_REC( rec, tags));
      DATA_REC( new_rec, btags) = SNetIntMapCopy( DATA_REC( rec, btags));
      SNetRecSetInterfaceId( new_rec, SNetRecGetInterfaceId( rec));
      SNetRecSetDataMode( new_rec, SNetRecGetDataMode( rec));
      SNetRecDetrefCopy( new_rec, rec);
      GenerateRecId( &DATA_REC( new_rec, rid) );		// generate a new Id for the new message
      break;
    case REC_sort_end:
      new_rec = SNetRecCreate( REC_DESCR( rec),  SORT_E_REC( rec, level),
                               SORT_E_REC( rec, num));
      break;
    case REC_terminate:
      new_rec = SNetRecCreate( REC_terminate);
      TERM_REC(new_rec, local) = TERM_REC(rec, local);
      break;
    default:
      new_rec = NULL;
      SNetRecUnknown(__func__, rec);
  }

  return new_rec;
}

void SNetRecDestroy( snet_record_t *rec)
{
  int name;
  snet_ref_t *field;

  switch (REC_DESCR( rec)) {
    case REC_data:
      RECORD_FOR_EACH_FIELD(rec, name, field) {
        SNetRefDestroy(field);
      }
      SNetRefMapDestroy( DATA_REC( rec, fields));
      SNetIntMapDestroy( DATA_REC( rec, tags));
      SNetIntMapDestroy( DATA_REC( rec, btags));
      (void) name;
      break;
    case REC_sync:
      {
        snet_variant_t *var = SYNC_REC( rec, outtype);
        if (var != NULL) {
          SNetVariantDestroy(var);
        }
      }
      break;
    case REC_collect:
      break;
    case REC_sort_end:
      break;
    case REC_terminate:
      break;
    case REC_trigger_initialiser:
      break;
    case REC_detref:
      break;
    case REC_observ:
      break;
    case REC_star_leader:
      break;
    case REC_wakeup:
      break;
    default:
      SNetRecUnknown(__func__, rec);
  }
  SNetMemFree( rec);
}

/*****************************************************************************/

snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec)
{
  return REC_DESCR( rec);
}

/*****************************************************************************/

snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetRecUnknown(__func__, rec);
  }

  DATA_REC( rec, interface_id) = id;
  return rec;
}

int SNetRecGetInterfaceId( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetRecUnknown(__func__, rec);
  }

  return DATA_REC( rec, interface_id);
}

/*****************************************************************************/

snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mod)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetRecUnknown(__func__, rec);
  }

  DATA_REC( rec, mode) = mod;
  return rec;
}

snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetRecUnknown(__func__, rec);
  }

  return DATA_REC( rec, mode);
}

/*****************************************************************************/

void SNetRecSetFlag( snet_record_t *rec)
{
  switch( REC_DESCR( rec)) {
  case REC_terminate:
    TERM_REC( rec, local) = true;
    break;
  default:
    SNetRecUnknown(__func__, rec);
  }
}
/*****************************************************************************/

snet_stream_t *SNetRecGetStream( snet_record_t *rec)
{
  snet_stream_t *result;

  switch( REC_DESCR( rec)) {
  case REC_sync:
    result = SYNC_REC( rec, input);
    break;
  case REC_collect:
    result = COLL_REC( rec, output);
    break;
  default:
    result = NULL;
    SNetRecUnknown(__func__, rec);
  }

  return result;
}

snet_variant_t *SNetRecGetVariant(snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sync) {
    SNetRecUnknown(__func__, rec);
  }
  return SYNC_REC( rec, outtype);
}

void SNetRecSetVariant(snet_record_t *rec, snet_variant_t *var)
{
  if (REC_DESCR( rec) != REC_sync) {
    SNetRecUnknown(__func__, rec);
  }
  if (SYNC_REC(rec, outtype)) SNetVariantDestroy(SYNC_REC(rec,outtype));
  SYNC_REC( rec, outtype) = (var!=NULL) ? SNetVariantCopy(var) : NULL;
}


/*****************************************************************************/

void SNetRecSetNum( snet_record_t *rec, int value)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetRecUnknown(__func__, rec);
  }

  SORT_E_REC( rec, num) = value;
}

int SNetRecGetNum( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetRecUnknown(__func__, rec);
  }

  return SORT_E_REC( rec, num);
}

void SNetRecSetLevel( snet_record_t *rec, int value)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetRecUnknown(__func__, rec);
  }

  SORT_E_REC( rec, level) = value;
}

int SNetRecGetLevel( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetRecUnknown(__func__, rec);
  }

  return SORT_E_REC( rec, level);
}


/*****************************************************************************/

void SNetRecSetTag( snet_record_t *rec, int name, int val)
{
  SNetIntMapSet(DATA_REC(rec, tags), name, val);
}

int SNetRecGetTag( snet_record_t *rec, int name)
{
  return SNetIntMapGet(DATA_REC(rec, tags), name);
}

int SNetRecTakeTag( snet_record_t *rec, int name)
{
  return SNetIntMapTake(DATA_REC(rec, tags), name);
}

bool SNetRecHasTag( snet_record_t *rec, int name)
{
  return SNetIntMapContains(DATA_REC(rec, tags), name);
}

void SNetRecRenameTag( snet_record_t *rec, int oldName, int newName)
{
  SNetIntMapRename(DATA_REC( rec, tags), oldName, newName);
}

/*****************************************************************************/

void SNetRecSetBTag( snet_record_t *rec, int name, int val)
{
  SNetIntMapSet(DATA_REC(rec, btags), name, val);
}

int SNetRecGetBTag( snet_record_t *rec, int name)
{
  return SNetIntMapGet(DATA_REC(rec, btags), name);
}

int SNetRecTakeBTag( snet_record_t *rec, int name)
{
  return SNetIntMapTake(DATA_REC(rec, btags), name);
}

bool SNetRecHasBTag( snet_record_t *rec, int name)
{
  return SNetIntMapContains(DATA_REC(rec, btags), name);
}

void SNetRecRenameBTag( snet_record_t *rec, int oldName, int newName)
{
  SNetIntMapRename(DATA_REC( rec, btags), oldName, newName);
}

/*****************************************************************************/

void SNetRecSetField( snet_record_t *rec, int name, snet_ref_t *val)
{
  SNetRefMapSet(DATA_REC(rec, fields), name, val);
}

snet_ref_t *SNetRecGetField( snet_record_t *rec, int name)
{
  return SNetRefCopy(SNetRefMapGet(DATA_REC(rec, fields), name));
}

snet_ref_t *SNetRecTakeField( snet_record_t *rec, int name)
{
  return SNetRefMapTake(DATA_REC(rec, fields), name);
}

bool SNetRecHasField( snet_record_t *rec, int name)
{
  return SNetRefMapContains(DATA_REC(rec, fields), name);
}

void SNetRecRenameField( snet_record_t *rec, int oldName, int newName)
{
  SNetRefMapRename(DATA_REC( rec, fields), oldName, newName);
}

/*****************************************************************************/

void SNetRecIdGet(snet_record_id_t *id, snet_record_t *from)
{
  if (REC_DESCR( from) != REC_data) {
    SNetRecUnknown(__func__, from);
  }
  *id = DATA_REC(from, rid);
}

/*****************************************************************************/


void SNetRecSerialise(
        snet_record_t *rec,
        void *buf,
        void (*packInts)(void*, int, int*),
        void (*packRefs)(void*, int, snet_ref_t**))
{
  snet_ref_t *val;
  int key, enumConversion;

  enumConversion = REC_DESCR(rec);
  packInts(buf, 1, &enumConversion);
  switch (REC_DESCR(rec)) {
    case REC_data:
      SNetIntMapSerialise(DATA_REC(rec, btags), buf, packInts, packInts);
      SNetIntMapSerialise(DATA_REC(rec, tags), buf, packInts, packInts);
      SNetRefMapSerialise(DATA_REC(rec, fields), buf, packInts, packRefs);

      enumConversion = DATA_REC( rec, mode);
      packInts(buf, 1, &enumConversion);

      enumConversion = DATA_REC( rec, interface_id);
      packInts(buf, 1, &enumConversion);

      SNetRecDetrefStackSerialise(rec, buf);

      MAP_DEQUEUE_EACH( DATA_REC( rec, fields), key, val) {
        SNetMemFree(val);
        (void) key;
      }
      break;
    case REC_sort_end:
      packInts(buf, 1, &SORT_E_REC(rec, level));
      packInts(buf, 1, &SORT_E_REC(rec, num));
      break;
    case REC_terminate:
      packInts(buf, 1, &TERM_REC(rec, local));
      break;
    case REC_trigger_initialiser:
      break;
    case REC_detref:
      SNetRecDetrefRecSerialise(rec, buf);
      break;
    default:
      SNetRecUnknown(__func__, rec);
  }

  SNetRecDestroy(rec);
}

snet_record_t *SNetRecDeserialise(
        void *buf,
        void (*unpackInts)(void*, int, int*),
        void (*unpackRefs)(void*, int, snet_ref_t**))
{
  int enumConversion;
  snet_record_t *result = NULL;

  unpackInts(buf, 1, &enumConversion);
  switch (enumConversion) {
    case REC_data:
      result = SNetRecCreate(enumConversion);
      SNetIntMapDeserialise(DATA_REC(result, btags), buf, unpackInts, unpackInts);
      SNetIntMapDeserialise(DATA_REC(result, tags), buf, unpackInts, unpackInts);
      SNetRefMapDeserialise(DATA_REC(result, fields), buf, unpackInts, unpackRefs);

      unpackInts(buf, 1, &enumConversion);
      DATA_REC( result, mode) = enumConversion;

      unpackInts(buf, 1, &enumConversion);
      DATA_REC( result, interface_id) = enumConversion;

      DATA_REC( result, detref) = NULL;
      SNetRecDetrefStackDeserialise(result, buf);
      break;
    case REC_sort_end:
      result = SNetRecCreate(enumConversion, 0, 0);
      unpackInts(buf, 1, &SORT_E_REC(result, level));
      unpackInts(buf, 1, &SORT_E_REC(result, num));
      break;
    case REC_terminate:
      result = SNetRecCreate(enumConversion);
      unpackInts(buf, 1, &TERM_REC(result, local));
      break;
    case REC_trigger_initialiser:
      result = SNetRecCreate(enumConversion);
      break;
    case REC_detref:
      result = SNetRecDetrefRecDeserialise(buf);
      break;
    default:
      SNetUtilDebugFatal("[%s]: Unknown control record description [%d].\n",
                          __func__, enumConversion);
  }

  return result;
}

/*****************************************************************************/

/* Convert a record to a string representation of its record descriptor type. */
const char* SNetRecTypeName(snet_record_t *rec)
{
  const snet_record_descr_t des = REC_DESCR(rec);

#define NAME(x) #x
#define TYPE(l) des == l ? NAME(l) :
  return
  TYPE(REC_data)
  TYPE(REC_sync)
  TYPE(REC_collect)
  TYPE(REC_sort_end)
  TYPE(REC_terminate)
  TYPE(REC_trigger_initialiser)
  TYPE(REC_detref)
  TYPE(REC_observ)
  TYPE(REC_star_leader)
  TYPE(REC_wakeup)
  NULL;
}

/* Print a message when a function encounters an unexpected record type. */
void SNetRecUnknown(const char *funcname, snet_record_t *rec)
{
  SNetUtilDebugFatal("[%s]: Unexpected record type: %s.", funcname,
                     SNetRecTypeName(rec));
}

/* Print a message when a function encounters an unexpected record type. */
void SNetRecUnknownEnt(const char *funcname, snet_record_t *rec,
                       snet_entity_t *ent)
{
  SNetUtilDebugFatalEnt(ent, "[%s]: Unexpected record type: %s.", funcname,
                        SNetRecTypeName(rec));
}

