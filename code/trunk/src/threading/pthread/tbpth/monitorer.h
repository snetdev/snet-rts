#ifndef _MONITORER_H_
#define _MONITORER_H_


//FIXME include the proper include file
struct snet_moninfo_t;


void SNetThreadingMonitoringInit(char *fname);


void SNetThreadingMonitoringCleanup(void);





void SNetThreadingMonitoringAppend(struct snet_moninfo_t *moninfo);


#endif /* _MONITORER_H_ */
