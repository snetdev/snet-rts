#ifndef _MONITORER_H_
#define _MONITORER_H_

#include "moninfo.h"
#include "entities.h"


void SNetThreadingMonitoringInit(char *fname);


void SNetThreadingMonitoringCleanup(void);


void SNetThreadingMonitoringAppend(snet_moninfo_t *moninfo, const char *label);


#endif /* _MONITORER_H_ */
