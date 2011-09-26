#ifndef IOMANAGERS_H
#define IOMANAGERS_H

typedef struct snet_dest snet_dest_t;

typedef enum {
  snet_rec,
  snet_update,
  snet_block,
  snet_unblock,
  snet_ref_set,
  snet_ref_fetch,
  snet_ref_update,
  snet_stop
} snet_comm_type_t;

#include "record.h"

typedef struct {
  snet_comm_type_t type;
  snet_record_t *rec;
  snet_dest_t *dest;
  snet_ref_t ref;
  void *data;
  int val;
} snet_msg_t;

/* To be implemented by distrib/implementation */
void SNetDistribNewDynamicCon(snet_dest_t *dest);

void SNetDistribUpdateBlocked(void);
void SNetDistribSendData(snet_ref_t ref, void *data, int node);

void SNetDistribBlockDest(snet_dest_t *dest);
void SNetDistribUnblockDest(snet_dest_t *dest);

void SNetDistribFetchRef(snet_ref_t *ref);
void SNetDistribUpdateRef(snet_ref_t *ref, int count);

snet_msg_t SNetDistribRecvMsg(void);
void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec);

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2);
snet_dest_t *SNetDestCopy(snet_dest_t *dest);
void SNetDestFree(snet_dest_t *dest);
#endif
