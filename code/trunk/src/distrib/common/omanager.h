#ifndef OMANAGER_H
#define OMANAGER_H

#include "distribcommon.h"
#include "threading.h"

void SNetOutputManagerInit(void);
void SNetOutputManagerStart(void);
void SNetOutputManagerStop(void);

void SNetOutputManagerNewOut(snet_dest_t *dest, snet_stream_t *stream);
void SNetOutputManagerBlock(snet_dest_t *dest);
void SNetOutputManagerUnblock(snet_dest_t *dest);
#endif
