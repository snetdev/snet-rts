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
  REC_trigger_initialiser
};

enum record_mode {
  MODE_textual,
  MODE_binary,
};

#include "distribution.h"
#include "snettypes.h"

/* returns true if the record matches the pattern. */
bool SNetRecPatternMatches();

snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...);
snet_record_t *SNetRecCopy( snet_record_t *rec);
void SNetRecDestroy( snet_record_t *rec);

snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec);

int SNetRecGetInterfaceId( snet_record_t *rec);
snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id);

snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec);
snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mode);

snet_stream_t *SNetRecGetStream( snet_record_t *rec);

void SNetRecSetNum( snet_record_t *rec, int value);
int SNetRecGetNum( snet_record_t *rec);
void SNetRecSetLevel( snet_record_t *rec, int value);
int SNetRecGetLevel( snet_record_t *rec);

void SNetRecSetTag( snet_record_t *rec, int id, int val);
int SNetRecGetTag( snet_record_t *rec, int id);
int SNetRecTakeTag( snet_record_t *rec, int id);
bool SNetRecHasTag( snet_record_t *rec, int id);

void SNetRecSetBTag( snet_record_t *rec, int id, int val);
int SNetRecGetBTag( snet_record_t *rec, int id);
int SNetRecTakeBTag( snet_record_t *rec, int id);
bool SNetRecHasBTag( snet_record_t *rec, int id);

void SNetRecSetField( snet_record_t *rec, int id, snet_ref_t *val);
snet_ref_t *SNetRecGetField( snet_record_t *rec, int id);
snet_ref_t *SNetRecTakeField( snet_record_t *rec, int id);
bool SNetRecHasField( snet_record_t *rec, int id);
#endif /* _RECORD_H_ */
