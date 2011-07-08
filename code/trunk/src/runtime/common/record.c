#include <stdarg.h>
#include "interface_functions.h"
#include "distribution.h"
#include "record.h"
#include "memfun.h"
#include "debug.h"
#include "map.h"
#include "variant.h"



/* ***************************************************************************/

bool SNetRecPatternMatches(snet_variant_t *pat, snet_record_t *rec)
{
  int val;

  VARIANT_FOR_EACH_FIELD(pat, val)
    if (!SNetRecHasField(rec, val)) {
      return false;
    }
  END_FOR

  VARIANT_FOR_EACH_TAG(pat, val)
    if (!SNetRecHasTag(rec, val)) {
      return false;
    }
  END_FOR

  VARIANT_FOR_EACH_BTAG(pat, val)
    if (!SNetRecHasBTag(rec, val)) {
      return false;
    }
  END_FOR

  return true;
}


void SNetRecFlowInherit( snet_variant_t *pat, snet_record_t *in_rec,
                             snet_record_t *out_rec)
{
  int name, val;
  void *field;
  snet_copy_fun_t copyfun = SNetInterfaceGet(DATA_REC( in_rec, interface_id))->copyfun;

  RECORD_FOR_EACH_FIELD(in_rec, name, field)
    if (!SNetVariantHasField( pat, name)) {
      SNetRecSetField( out_rec, name, copyfun(field));
    }
  END_FOR

  RECORD_FOR_EACH_TAG(in_rec, name, val)
    if (!SNetVariantHasTag( pat, name)) {
      SNetRecSetTag( out_rec, name, val);
    }
  END_FOR
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
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, data_rec) = SNetMemAlloc( sizeof( data_rec_t));
      DATA_REC( rec, btags) = SNetIntMapCreate(0);
      DATA_REC( rec, tags) = SNetIntMapCreate(0);
      DATA_REC( rec, fields) = SNetVoidMapCreate(0);
      DATA_REC( rec, mode) = MODE_binary;
      DATA_REC( rec, interface_id) = 0;
      break;
    case REC_trigger_initialiser:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sync:
      {
        RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
        RECORD( rec, sync_rec) = SNetMemAlloc( sizeof( sync_rec_t));
        SYNC_REC( rec, input) = va_arg( args, snet_stream_t *);
        SYNC_REC( rec, outtype) = NULL;
      }
      break;
    case REC_collect:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, coll_rec) = SNetMemAlloc( sizeof( coll_rec_t));
      COLL_REC( rec, output) = va_arg( args, snet_stream_t*);
      break;
    case REC_terminate:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sort_end:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sort_end_rec) = SNetMemAlloc( sizeof( sort_end_t));
      SORT_E_REC( rec, level) = va_arg( args, int);
      SORT_E_REC( rec, num) = va_arg( args, int);
      break;
    case REC_source:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, source_rec) = SNetMemAlloc( sizeof( source_rec_t));
      SOURCE_REC( rec, loc) = SNetLocvecCopy(va_arg( args, snet_locvec_t*));
      break;
    default:
      SNetUtilDebugFatal("Unknown control record description. [%d]", descr);
      break;
  }
  va_end( args);

  return rec;
}

snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  snet_record_t *new_rec;
  snet_copy_fun_t copyfun;

  switch (REC_DESCR( rec)) {
    case REC_data:
      copyfun = SNetInterfaceGet(DATA_REC( rec, interface_id))->copyfun;
      new_rec = SNetRecCreate( REC_data);
      DATA_REC( new_rec, fields) = SNetVoidMapDeepCopy(DATA_REC( rec, fields),
                                                       copyfun);

      DATA_REC( new_rec, tags) = SNetIntMapCopy( DATA_REC( rec, tags));
      DATA_REC( new_rec, btags) = SNetIntMapCopy( DATA_REC( rec, btags));
      SNetRecSetInterfaceId( new_rec, SNetRecGetInterfaceId( rec));
      SNetRecSetDataMode( new_rec, SNetRecGetDataMode( rec));
      break;
    case REC_sort_end:
      new_rec = SNetRecCreate( REC_DESCR( rec),  SORT_E_REC( rec, level),
                               SORT_E_REC( rec, num));
      break;
    case REC_terminate:
      new_rec = SNetRecCreate( REC_terminate);
      break;
    default:
      SNetUtilDebugFatal("Can't copy record of type %d", REC_DESCR( rec));
      break;
  }

  return new_rec;
}

void SNetRecDestroy( snet_record_t *rec)
{
  int name;
  void *field;
  snet_free_fun_t freefun;

  switch (REC_DESCR( rec)) {
    case REC_data:
      freefun = SNetInterfaceGet(DATA_REC( rec, interface_id))->freefun;
      RECORD_FOR_EACH_FIELD(rec, name, field)
        freefun(field);
      END_FOR
      SNetVoidMapDestroy( DATA_REC( rec, fields));
      SNetIntMapDestroy( DATA_REC( rec, tags));
      SNetIntMapDestroy( DATA_REC( rec, btags));
      SNetMemFree( RECORD( rec, data_rec));
      break;
    case REC_sync:
      {
        snet_variant_t *var = SYNC_REC( rec, outtype);
        if (var != NULL) {
          SNetVariantDestroy(var);
        }
        SNetMemFree( RECORD( rec, sync_rec));
      }
      break;
    case REC_collect:
      SNetMemFree( RECORD( rec, coll_rec));
      break;
    case REC_sort_end:
      SNetMemFree( RECORD( rec, sort_end_rec));
      break;
    case REC_terminate:
      break;
    case REC_trigger_initialiser:
      break;
    case REC_source:
      SNetLocvecDestroy( SOURCE_REC( rec, loc));
      SNetMemFree( RECORD( rec, source_rec));
      break;
    default:
      SNetUtilDebugFatal("Unknown record description, in SNetRecDestroy");
      break;
  }
  SNetMemFree( RECPTR( rec));
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
    SNetUtilDebugFatal("SNetRecSetInterfaceId only accepts data records (%d)",
                       REC_DESCR(rec));
  }

  DATA_REC( rec, interface_id) = id;
  return rec;
}

int SNetRecGetInterfaceId( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetUtilDebugFatal("SNetRecGetInterfaceId only accepts data records (%d)",
                       REC_DESCR(rec));
  }

  return DATA_REC( rec, interface_id);
}

/*****************************************************************************/

snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mod)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetUtilDebugFatal("SNetRecSetDataMode only accepts data records (%d)",
                       REC_DESCR(rec));
  }

  DATA_REC( rec, mode) = mod;
  return rec;
}

snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_data) {
    SNetUtilDebugFatal("SNetRecGetDataMode only accepts data records (%d)",
                       REC_DESCR(rec));
  }

  return DATA_REC( rec, mode);
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
    SNetUtilDebugFatal("Wrong type in SNetRecGetStream() (%d)", REC_DESCR(rec));
    break;
  }

  return result;
}

snet_variant_t *SNetRecGetVariant(snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sync) {
    SNetUtilDebugFatal("Wrong type in SNetRecGetVariant() (%d)", REC_DESCR(rec));
  }
  return SYNC_REC( rec, outtype);
}

void SNetRecSetVariant(snet_record_t *rec, snet_variant_t *var)
{
  if (REC_DESCR( rec) != REC_sync) {
    SNetUtilDebugFatal("Wrong type in SNetRecSetVariant() (%d)", REC_DESCR(rec));
  }
  if (SYNC_REC(rec, outtype)) SNetVariantDestroy(SYNC_REC(rec,outtype));
  SYNC_REC( rec, outtype) = (var!=NULL) ? SNetVariantCopy(var) : NULL;
}

/*****************************************************************************/

snet_locvec_t *SNetRecGetLocvec(snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_source) {
    SNetUtilDebugFatal("Wrong type in SNetRecGetLocvec() (%d)", REC_DESCR(rec));
  }
  return SOURCE_REC( rec, loc);
}

/*****************************************************************************/

void SNetRecSetNum( snet_record_t *rec, int value)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetUtilDebugFatal("Wrong type in SNetRecSetNum() (%d)", REC_DESCR( rec));
  }

  SORT_E_REC( rec, num) = value;
}

int SNetRecGetNum( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetUtilDebugFatal("Wrong type in SNetRecGetNum() (%d)", REC_DESCR( rec));
  }

  return SORT_E_REC( rec, num);
}

void SNetRecSetLevel( snet_record_t *rec, int value)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetUtilDebugFatal("Wrong type in SNetRecSetLevel() (%d)", REC_DESCR( rec));
  }

  SORT_E_REC( rec, level) = value;
}

int SNetRecGetLevel( snet_record_t *rec)
{
  if (REC_DESCR( rec) != REC_sort_end) {
    SNetUtilDebugFatal("Wrong type in SNetRecSetLevel() (%d)", REC_DESCR( rec));
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

void SNetRecSetField( snet_record_t *rec, int name, void *val)
{
  SNetVoidMapSet(DATA_REC(rec, fields), name, val);
}

void *SNetRecGetField( snet_record_t *rec, int name)
{
  snet_copy_fun_t copyfun = SNetInterfaceGet(DATA_REC( rec, interface_id))->copyfun;
  return copyfun(SNetVoidMapGet(DATA_REC(rec, fields), name));
}

void *SNetRecTakeField( snet_record_t *rec, int name)
{
  return SNetVoidMapTake(DATA_REC(rec, fields), name);
}

bool SNetRecHasField( snet_record_t *rec, int name)
{
  return SNetVoidMapContains(DATA_REC(rec, fields), name);
}

void SNetRecRenameField( snet_record_t *rec, int oldName, int newName)
{
  SNetVoidMapRename(DATA_REC( rec, fields), oldName, newName);
}

/*****************************************************************************/



