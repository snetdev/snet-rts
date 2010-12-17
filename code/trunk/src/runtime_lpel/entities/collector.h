#ifndef _COLLECTORS_H_
#define _COLLECTORS_H_

#include "stream.h"
#include "info.h"
stream_t *CollectorCreate( int num, stream_t **instreams, snet_info_t *info);
#endif
