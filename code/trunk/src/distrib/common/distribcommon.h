#ifndef DISTRIBCOMMON_H
#define DISTRIBCOMMON_H

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
#include "dest.h"

typedef struct {
  snet_comm_type_t type;
  snet_record_t *rec;
  snet_dest_t *dest;
  snet_ref_t ref;
  void *data;
  int val;
} snet_msg_t;

/* To be implemented by distrib/implementation */
void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info);
void SNetDistribLocalStop(void);

snet_msg_t SNetDistribRecvMsg(void);
void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec);

void SNetDistribBlockDest(snet_dest_t *dest);
void SNetDistribUnblockDest(snet_dest_t *dest);

void SNetDistribUpdateBlocked(void);
void SNetDistribSendData(snet_ref_t ref, void *data, int node);
#endif
