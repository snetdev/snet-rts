#ifndef _RECORD_H_
#define _RECORD_H_

#include <stddef.h>

typedef struct record snet_record_t;
typedef union record_types snet_record_types_t;

typedef enum {
  REC_data=1100,
  REC_sync,
  REC_collect,
  REC_sort_end,
  REC_terminate,
  REC_trigger_initialiser,
  REC_detref,
  REC_observ,
  REC_star_leader,
  REC_wakeup,
} snet_record_descr_t;

typedef enum {
  MODE_textual,
  MODE_binary,
} snet_record_mode_t;


#define SNET_REC_SUBID_NUM  2

typedef struct {
  int subid[SNET_REC_SUBID_NUM];
} snet_record_id_t;




/* macros for record datastructure */
#define REC_DESCR( name) name->rec_descr
#define RECPTR( name) name->rec
#define RECORD( name, type) RECPTR( name).type

#define DATA_REC( name, component) RECORD( name, data_rec).component
#define SYNC_REC( name, component) RECORD( name, sync_rec).component
#define SORT_E_REC( name, component) RECORD( name, sort_end_rec).component
#define TERM_REC( name, component) RECORD( name, terminate_rec).component
#define COLL_REC( name, component) RECORD( name, coll_rec).component
#define DETREF_REC( name, component) RECORD( name, detref_rec).component
#define OBSERV_REC( name, component) RECORD( name, observ_rec).component
#define LEADER_REC( name, component) RECORD( name, leader_rec).component

#include "distribution.h"
#include "map.h"
#include "variant.h"
#include "stream.h"
#include "bool.h"
#include "locvec.h"


#define LIST_NAME_H RecId /* SNetRecIdListFUNC */
#define LIST_TYPE_NAME_H recid
#define LIST_VAL_H snet_record_id_t
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H


typedef struct {
  snet_int_map_t *tags;
  snet_int_map_t *btags;
  snet_ref_map_t *fields;
  int interface_id;
  snet_record_mode_t mode;
  snet_record_id_t rid;           /* system-wide unique id */
  struct snet_stack *detref;
} data_rec_t;

typedef struct {
  snet_stream_t *input;
  snet_variant_t *outtype;
} sync_rec_t;

typedef struct {
  int num;
  int level;
} sort_end_rec_t;

typedef struct {
  bool local; /* a flag that is set only at a local GC cleanup */
} terminate_rec_t;

typedef struct {
  snet_stream_t *output;
} coll_rec_t;

typedef struct detref_rec {
  long                   seqnr;
  struct landing        *leave;
  struct detref         *detref;
} detref_rec_t;

typedef struct observ_rec {
  int                            oid;
  snet_record_t                 *rec;
  struct snet_stream_desc_t    **desc;
} observ_rec_t;

typedef struct leader_rec {
  long  counter;
  long  seqnr;
} leader_rec_t;

union record_types {
  data_rec_t data_rec;
  sync_rec_t sync_rec;
  coll_rec_t coll_rec;
  sort_end_rec_t sort_end_rec;
  terminate_rec_t terminate_rec;
  detref_rec_t detref_rec;
  observ_rec_t observ_rec;
  leader_rec_t leader_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t rec;
};

#define RECORD_FOR_EACH_TAG(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, tags), name, val)
#define RECORD_FOR_EACH_BTAG(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, btags), name, val)
#define RECORD_FOR_EACH_FIELD(rec, name, val) MAP_FOR_EACH(DATA_REC(rec, fields), name, val)

/* return number of created records */
unsigned SNetGetRecCounter(void);

/* compares two record ids */
bool SNetRecordIdEquals (snet_record_id_t rid1, snet_record_id_t rid2);

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

void SNetRecSetFlag( snet_record_t *rec);

snet_stream_t *SNetRecGetStream( snet_record_t *rec);

snet_variant_t *SNetRecGetVariant(snet_record_t *rec);
void SNetRecSetVariant(snet_record_t *rec, snet_variant_t *var);


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

void SNetRecSetField( snet_record_t *rec, int id, snet_ref_t *val);
snet_ref_t *SNetRecGetField( snet_record_t *rec, int id);
snet_ref_t *SNetRecTakeField( snet_record_t *rec, int id);
bool SNetRecHasField( snet_record_t *rec, int id);
void SNetRecRenameField( snet_record_t *rec, int id, int newId);



void SNetRecIdGet(snet_record_id_t *id, snet_record_t *from);


void SNetRecSerialise(
        snet_record_t *rec,
        void *buf,
        void (*packInts)(void*, int, int*),
        void (*packRefs)(void*, int, snet_ref_t**)
);

snet_record_t *SNetRecDeserialise(
        void *buf,
        void (*unpackInts)(void*, int, int*),
        void (*unpackRefs)(void*, int, snet_ref_t**)
);

/* Copy the stack of detref references from one record to another. */
void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *old_rec);

const char* SNetRecTypeName(snet_record_t *rec);

void SNetRecUnknown(const char *funcname, snet_record_t *rec);

void SNetRecUnknownTask(const char *funcname, snet_record_t *rec);

#endif /* _RECORD_H_ */
