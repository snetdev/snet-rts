#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "threading.h"
#include "debug.h"
#include "bool.h"
#include "memfun.h"
#include "distribution.h"

static void PrintDebugMessage(char *msg, char *category,
  va_list arg, const char *name)
{
  if (name) {
    fprintf(stderr, "(SNET %s (NODE %d THREAD %s)", category,
            SNetDistribGetNodeId(), name);
  } else {
    fprintf(stderr, "(SNET %s (NODE %d THREAD %lu)", category,
            SNetDistribGetNodeId(), (unsigned long) pthread_self());
  }

  vfprintf(stderr, msg, arg);
  fprintf(stderr, ")\n");
  fflush(stderr);
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
void SNetUtilDebugFatalTask(char* msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "FATAL", arg, SNetThreadingGetName());
  va_end(arg);
  abort();
}

/**
 * Print a warning/notice message on stderr
 */
void SNetUtilDebugNoticeTask(char *msg, ...)
{
  va_list arg;

  va_start(arg, msg);
  PrintDebugMessage(msg, "NOTICE", arg, SNetThreadingGetName());
  va_end(arg);
}
