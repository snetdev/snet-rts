#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "locvec.h"
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
  va_list arg, snet_locvec_t *locvec)
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

  if (locvec != NULL) {
    ret = snprintf(temp, len, "(SNET %s (NODE %d in ",
        category,
        SNetDistribGetNodeId()
        );
    CheckAndUpdate( &ret, &len, &num);

    ret = SNetLocvecPrint(temp+num, len, locvec);
    CheckAndUpdate( &ret, &len, &num);

    ret = snprintf(temp+num, len, ") ");
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
void SNetUtilDebugFatalLoc(snet_locvec_t *locvec, char* msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "FATAL", arg, locvec);
  va_end(arg);
  abort();
}

/**
 * Print a warning/notice message on stderr
 */
void SNetUtilDebugNoticeLoc(snet_locvec_t *locvec, char *msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "NOTICE", arg, locvec);
  va_end(arg);
}


