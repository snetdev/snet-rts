#include "distribution.h"
#include "record.h"
#include "debug.h"

#define NAME int
#define VAL int
#include "map.c.h"
#undef VAL
#undef NAME

#define NAME ref
#define VAL snet_ref_t*
#include "map.c.h"
#undef VAL
#undef NAME

/* macros for record datastructure */
#define REC_DESCR( name) name->rec_descr
#define RECPTR( name) name->rec
#define RECORD( name, type) RECPTR( name)->type

#define DATA_REC( name, component) RECORD( name, data_rec)->component
#define SYNC_REC( name, component) RECORD( name, sync_rec)->component
#define SORT_E_REC( name, component) RECORD( name, sort_end_rec)->component
#define TERMINATE_REC( name, component) RECORD( name, terminate_hnd)->component
#define COLL_REC( name, component) RECORD( name, coll_rec)->component

typedef struct {
  snet_map_int_t *tags;
  snet_map_int_t *btags;
  snet_map_ref_t *fields;
  int interface_id;
  snet_record_mode_t mode;
} data_rec_t;

typedef struct {
  snet_stream_t *input;
} sync_rec_t;

typedef struct {
  int num;
  int level;
} sort_end_t;

typedef struct {
  /* empty */
} terminate_rec_t;

typedef struct {
  snet_stream_t *output;
} coll_rec_t;

union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  coll_rec_t *coll_rec;
  sort_end_t *sort_end_rec;
  terminate_rec_t *terminate_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
};

/* ***************************************************************************/

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
      DATA_REC( rec, btags) = SNetMapintCreate(0, -1);
      DATA_REC( rec, tags) = SNetMapintCreate(0, -1);
      DATA_REC( rec, fields) = SNetMaprefCreate(0, NULL);
      DATA_REC( rec, mode) = MODE_binary;
      DATA_REC( rec, interface_id) = 0;
      break;
    case REC_trigger_initialiser:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sync:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sync_rec) = SNetMemAlloc( sizeof( sync_rec_t));
      SYNC_REC( rec, input) = va_arg( args, snet_stream_t *);
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
    default:
      SNetUtilDebugFatal("Unknown control record destription. [%d]", descr);
  }
  va_end( args);

  return rec;
}

snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  snet_record_t *new_rec;

  switch (REC_DESCR( rec)) {
    case REC_data:
      new_rec = SNetRecCreate( REC_data);
      /* FIXME: copy tags, btags, fields */
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
  }

  return new_rec;
}

void SNetRecDestroy( snet_record_t *rec)
{
  switch (REC_DESCR( rec)) {
    case REC_data:
      /* FIXME: free all references */
      /* FIXME: free btags, tags, fields */
      SNetMemFree( RECORD( rec, data_rec));
      break;
    case REC_sync:
      SNetMemFree( RECORD( rec, sync_rec));
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
  SNetMapintSet(DATA_REC(rec, tags), name, val);
}

int SNetRecGetTag( snet_record_t *rec, int name)
{
  return SNetMapintGet(DATA_REC(rec, tags), name);
}

int SNetRecTakeTag( snet_record_t *rec, int name)
{
  return SNetMapintTake(DATA_REC(rec, tags), name);
}

bool SNetRecHasTag( snet_record_t *rec, int name)
{
  return SNetRecGetTag(rec, name) != -1;
}

/*****************************************************************************/

void SNetRecSetBTag( snet_record_t *rec, int name, int val)
{
  SNetMapintSet(DATA_REC(rec, btags), name, val);
}

int SNetRecGetBTag( snet_record_t *rec, int name)
{
  return SNetMapintGet(DATA_REC(rec, btags), name);
}

int SNetRecTakeBTag( snet_record_t *rec, int name)
{
  return SNetMapintTake(DATA_REC(rec, btags), name);
}

bool SNetRecHasBTag( snet_record_t *rec, int name)
{
  return SNetRecGetBTag(rec, name) != -1;
}

/*****************************************************************************/

void SNetRecSetField( snet_record_t *rec, int name, snet_ref_t *val)
{
  SNetMaprefSet(DATA_REC(rec, fields), name, val);
}

snet_ref_t *SNetRecGetField( snet_record_t *rec, int name)
{
  return SNetMaprefGet(DATA_REC(rec, fields), name);
}

snet_ref_t *SNetRecTakeField( snet_record_t *rec, int name)
{
  return SNetMaprefTake(DATA_REC(rec, fields), name);
}

bool SNetRecHasField( snet_record_t *rec, int name)
{
  return SNetRecGetField(rec, name) != NULL;
}
