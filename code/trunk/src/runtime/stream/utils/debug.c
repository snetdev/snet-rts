#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "entities.h"
#include "debug.h"
#include "bool.h"
#include "memfun.h"
#include "distribution.h"

#define MAX_SIZE 256


/**
 * Helper function to check overflow and
 * keep track of written chars and space left
 */
static inline void CheckAndUpdate(int *ret, int *len, int *num)
{
  assert( *ret >= 0 );
  if(*ret >= *len) {
    SNetUtilDebugFatal("Debug message too long!");
  }

  /* update counters */
  *num += *ret; *len -= *ret;
}


static void PrintDebugMessage(char *msg, char *category,
  va_list arg, snet_entity_t *ent)
{
  char *temp;
  int num, ret, len;

  /* NOTICE: the text is first printed into buffer to prevent it from
   * breaking in to multiple pieces when printing to stderr!
   */

  len = strlen(msg) + MAX_SIZE;
  num = 0;

  temp = SNetMemAlloc(sizeof(char) * len);
  //memset(temp, 0, len);

  if (ent != NULL) {
    ret = snprintf(temp, len, "(SNET %s (NODE %d in %s)",
        category,
        SNetDistribGetNodeId(),
        SNetEntityStr(ent)
        );
    CheckAndUpdate( &ret, &len, &num);
  } else {
    ret = snprintf(temp, len, "(SNET %s (NODE %d THREAD %lu) ",
        category,
        SNetDistribGetNodeId(),
        (unsigned long) pthread_self()
        );
    CheckAndUpdate( &ret, &len, &num);
  }

  ret = vsnprintf(temp+num, len, msg, arg);
  CheckAndUpdate( &ret, &len, &num);

  ret = snprintf(temp+num, len, ")\n");
  CheckAndUpdate( &ret, &len, &num);

  /* finally print to stderr */
  fputs(temp, stderr);
  fflush(stderr);

  SNetMemFree(temp);
}



/**
 * Print an error message on stderr and abort the program.
 */
void SNetUtilDebugFatal(char* msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "FATAL", arg, NULL);
  va_end(arg);
  abort();
}

/**
 * Print a warning/notice message on stderr
 */
void SNetUtilDebugNotice(char *msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "NOTICE", arg, NULL);
  va_end(arg);
}


/**
 * Print an error message on stderr and abort the program.
 */
void SNetUtilDebugFatalEnt(snet_entity_t *ent, char* msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "FATAL", arg, ent);
  va_end(arg);
  abort();
}

/**
 * Print a warning/notice message on stderr
 */
void SNetUtilDebugNoticeEnt(snet_entity_t *ent, char *msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "NOTICE", arg, ent);
  va_end(arg);
}


