#ifndef OMANAGER_H
#define OMANAGER_H

#include "iomanagers.h"
#include "threading.h"

void SNetOutputManagerInit(void);
void SNetOutputManagerStart(void);
void SNetOutputManagerStop(void);

void SNetOutputManagerNewOut(snet_dest_t *dest, snet_stream_t *stream);
void SNetOutputManagerBlockDest(snet_dest_t *dest);
void SNetOutputManagerUnblockDest(snet_dest_t *dest);
#endif
