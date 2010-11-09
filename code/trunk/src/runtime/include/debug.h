#ifndef DEBUG_H
#define DEBUG_H

#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "assert.h"
#include "record.h"

/* prints a record in readable format 
 */
extern char* SNetUtilDebugDumpRecord(snet_record_t *data, char* storage);

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
#endif
