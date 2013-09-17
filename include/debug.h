#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "entities.h"

#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

/*
 * reports an error and terminates the application after that.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugFatal(char* message, ...)
            __attribute__ ((format (printf, 1, 2)))
            __attribute__ ((noreturn));

/*
 * reports an error.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugNotice(char* message, ...)
            __attribute__ ((format (printf, 1, 2)));



extern void SNetUtilDebugFatalEnt(snet_entity_t *ent, char* msg, ...)
            __attribute__ ((format (printf, 2, 3)))
            __attribute__ ((noreturn));

extern void SNetUtilDebugNoticeEnt(snet_entity_t *ent, char* msg, ...)
            __attribute__ ((format (printf, 2, 3)));


#endif /* _DEBUG_H_ */
