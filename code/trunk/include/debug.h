#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "locvec.h"

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



extern void SNetUtilDebugFatalLoc(snet_locvec_t *locvec, char* msg, ...);

extern void SNetUtilDebugNoticeLoc(snet_locvec_t *locvec, char* msg, ...);


#endif /* _DEBUG_H_ */
