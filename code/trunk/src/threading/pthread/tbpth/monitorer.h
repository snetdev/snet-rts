#ifndef _MONITORER_H_
#define _MONITORER_H_

#include "moninfo.h"
#include "locvec.h"


void SNetThreadingMonitoringInit(char *fname);


void SNetThreadingMonitoringCleanup(void);


void SNetThreadingMonitoringAppend(snet_moninfo_t *moninfo, snet_locvec_t *loc);


#endif /* _MONITORER_H_ */
