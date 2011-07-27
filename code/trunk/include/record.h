#ifndef _RECORD_H_
#define _RECORD_H_

typedef struct record snet_record_t;
typedef union record_types snet_record_types_t;
typedef enum record_descr snet_record_descr_t;
typedef enum record_mode snet_record_mode_t;

enum record_descr {
  REC_data,
  REC_sync,
  REC_collect,
  REC_sort_end,
  REC_terminate,
  REC_trigger_initialiser,
  REC_source
};

enum record_mode {
  MODE_textual,
  MODE_binary,
};


/* macros for record datastructure */
#define REC_DESCR( name) name->rec_descr
#define RECPTR( name) name->rec
#define RECORD( name, type) RECPTR( name)->type

#define DATA_REC( name, component) RECORD( name, data_rec)->component
#define SYNC_REC( name, component) RECORD( name, sync_rec)->component
#define SORT_E_REC( name, component) RECORD( name, sort_end_rec)->component
#define TERMINATE_REC( name, component) RECORD( name, terminate_hnd)->component
#define COLL_REC( name, component) RECORD( name, coll_rec)->component
#define SOURCE_REC( name, component) RECORD( name, source_rec)->component

#include "stream.h"
#include "map.h"
#include "bool.h"
#include "variant.h"
#include "locvec.h"

typedef struct {
  snet_int_map_t *tags;
  snet_int_map_t *btags;
  snet_void_map_t *fields;
  int interface_id;
  snet_record_mode_t mode;
} data_rec_t;

typedef struct {
  snet_stream_t *input;
  snet_variant_t *outtype;
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

typedef struct {
  snet_locvec_t *loc;
} source_rec_t;


union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  coll_rec_t *coll_rec;
  sort_end_t *sort_end_rec;
  terminate_rec_t *terminate_rec;
  source_rec_t *source_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
};

#define RECORD_FOR_EACH_TAG(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, tags), name, val)
#define RECORD_FOR_EACH_BTAG(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, btags), name, val)
#define RECORD_FOR_EACH_FIELD(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, fields), name, val)


#include "variant.h"
#include "distribution.h"

/* returns true if the record matches the pattern. */
bool SNetRecPatternMatches(snet_variant_t *pat, snet_record_t *rec);
void SNetRecFlowInherit( snet_variant_t *pat, snet_record_t *in_rec,
                             snet_record_t *out_rec);

snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...);
snet_record_t *SNetRecCopy( snet_record_t *rec);
void SNetRecDestroy( snet_record_t *rec);

snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec);

int SNetRecGetInterfaceId( snet_record_t *rec);
snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id);

snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec);
snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mode);

snet_stream_t *SNetRecGetStream( snet_record_t *rec);

snet_variant_t *SNetRecGetVariant(snet_record_t *rec);
void SNetRecSetVariant(snet_record_t *rec, snet_variant_t *var);

snet_locvec_t *SNetRecGetLocvec( snet_record_t *rec);

void SNetRecSetNum( snet_record_t *rec, int value);
int SNetRecGetNum( snet_record_t *rec);
void SNetRecSetLevel( snet_record_t *rec, int value);
int SNetRecGetLevel( snet_record_t *rec);

void SNetRecSetTag( snet_record_t *rec, int id, int val);
int SNetRecGetTag( snet_record_t *rec, int id);
int SNetRecTakeTag( snet_record_t *rec, int id);
bool SNetRecHasTag( snet_record_t *rec, int id);
void SNetRecRenameTag( snet_record_t *rec, int id, int newId);

void SNetRecSetBTag( snet_record_t *rec, int id, int val);
int SNetRecGetBTag( snet_record_t *rec, int id);
int SNetRecTakeBTag( snet_record_t *rec, int id);
bool SNetRecHasBTag( snet_record_t *rec, int id);
void SNetRecRenameBTag( snet_record_t *rec, int id, int newId);

void SNetRecSetField( snet_record_t *rec, int id, void *val);
void *SNetRecGetField( snet_record_t *rec, int id);
void *SNetRecTakeField( snet_record_t *rec, int id);
bool SNetRecHasField( snet_record_t *rec, int id);
void SNetRecRenameField( snet_record_t *rec, int id, int newId);

void SNetRecSerialise( snet_record_t *rec, void (*serialise)(int, int*));
snet_record_t *SNetRecDeserialise(void (*deserialise)(int, int*));

#endif /* _RECORD_H_ */
