#ifndef _COLLECTORS_H_
#define _COLLECTORS_H_

#include "snettypes.h"
#include "info.h"
snet_stream_t *CollectorCreate( int num, snet_stream_t **instreams, bool dynamic, snet_info_t *info);
#endif
