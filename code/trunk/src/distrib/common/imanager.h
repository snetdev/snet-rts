#ifndef IMANAGER_H
#define IMANAGER_H

#include "distribcommon.h"
#include "threading.h"

typedef struct snet_buffer snet_buffer_t;

void SNetInputManagerInit(void);
void SNetInputManagerStart(void);
void SNetInputManagerNewIn(snet_dest_t *dest, snet_stream_t *stream);
#endif
