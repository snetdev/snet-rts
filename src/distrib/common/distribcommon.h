#ifndef DISTRIBCOMMON_H
#define DISTRIBCOMMON_H

#include "memfun.h"
#include "record.h"

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

typedef struct {
  int node,
      dest,
      parent,
      parentNode,
      dynamicIndex,
      dynamicLoc;
} snet_dest_t;

typedef struct {
  snet_dest_t dest;
  snet_stream_t *stream;
} snet_tuple_t;

typedef struct {
  snet_comm_type_t type;
  snet_record_t *rec;
  snet_dest_t dest;
  snet_ref_t *ref;
  uintptr_t data;
  int val;
} snet_msg_t;

inline static bool SNetDestCompare(snet_dest_t d1, snet_dest_t d2)
{
  return d1.node == d2.node && d1.dest == d2.dest &&
         d1.parent == d2.parent && d1.dynamicIndex == d2.dynamicIndex;
}

inline static snet_dest_t *SNetDestCopy(snet_dest_t *d)
{
  snet_dest_t *result = SNetMemAlloc(sizeof(snet_dest_t));
  *result = *d;
  return result;
}

inline static bool SNetTupleCompare(snet_tuple_t t1, snet_tuple_t t2)
{
  (void) t1; /* NOT USED */
  (void) t2; /* NOT USED */
  return false;
}

/* To be implemented by distrib/implementation */
void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info);
void SNetDistribLocalStop(void);

snet_msg_t SNetDistribRecvMsg(void);
void SNetDistribSendRecord(snet_dest_t dest, snet_record_t *rec);

void SNetDistribBlockDest(snet_dest_t dest);
void SNetDistribUnblockDest(snet_dest_t dest);

void SNetDistribUpdateBlocked(void);
void SNetDistribSendData(snet_ref_t *ref, void *data, void *dest);
#endif
