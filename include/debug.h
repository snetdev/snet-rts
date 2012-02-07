#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "entities.h"

/*
 * reports an error and terminates the application after that.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugFatal(char* message, ...);

/*
 * reports an error.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugNotice(char* message, ...);



extern void SNetUtilDebugFatalEnt(snet_entity_t *ent, char* msg, ...);

extern void SNetUtilDebugNoticeEnt(snet_entity_t *ent, char* msg, ...);


#endif /* _DEBUG_H_ */
