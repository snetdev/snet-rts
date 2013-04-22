#ifndef _GLUE_SNET_H
#define _GLUE_SNET_H

#define SNET_SOURCE_PREFIX 		"snet_source"
#define SNET_SINK_PREFIX 			"snet_sink"

#include "threading.h"

size_t GetStacksize(snet_entity_descr_t descr);
void *EntityTask(void *arg);
char *strnstr(const char *s, const char *find, size_t slen);

#endif  /* _GLUE_SNET_H */

