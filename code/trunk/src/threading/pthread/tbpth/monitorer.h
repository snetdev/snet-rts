#ifndef _MONITORER_H_
#define _MONITORER_H_

#include "moninfo.h"


void SNetThreadingMonitoringInit(char *fname);


void SNetThreadingMonitoringCleanup(void);





void SNetThreadingMonitoringAppend(snet_moninfo_t *moninfo);


#endif /* _MONITORER_H_ */
