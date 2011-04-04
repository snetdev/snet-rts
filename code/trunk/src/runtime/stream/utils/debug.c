#include <pthread.h>
#include <string.h>

#include "debug.h"
#include "memfun.h"
#include "distribution.h"

#define MAX_SIZE 256

extern void SNetUtilDebugFatal(char* m, ...) {
  va_list p;
  char *temp;
  int num;
  int ret;

  /* NOTICE: the text is first printed into buffer to prevent it from
   * breaking in to multiple pieces when printing to stderr!
   */

  temp = SNetMemAlloc(sizeof(char) * (strlen(m) + MAX_SIZE));
  memset(temp, 0, strlen(m) + MAX_SIZE);

  va_start(p, m);

  num = sprintf(temp, "(SNET FATAL (NODE %d THREAD %lu) ", SNetNodeLocation, pthread_self());
  ret = vsprintf(temp + num, m, p);

  if(ret < 0) {
    SNetUtilDebugFatal("Debug message too long!");
  }

  num += ret;

  ret = sprintf(temp + num, ")\n");

  if(ret < 0) {
    SNetUtilDebugFatal("Debug message too long!");
  }

  fputs(temp, stderr);
  fflush(stderr);
  va_end(p);

  abort();
}

extern void SNetUtilDebugNotice(char *m, ...) {
  va_list p;
  char *temp;
  int num;
  int ret;

  /* NOTICE: the text is first printed into buffer to prevent it from
   * breaking in to multiple pieces when printing to stderr!
   */

  temp = SNetMemAlloc(sizeof(char) * (strlen(m) + MAX_SIZE));
  memset(temp, 0, strlen(m) + MAX_SIZE);

  va_start(p, m);
  num = sprintf(temp, "(SNET NOTICE (NODE %d THREAD %lu) ", SNetNodeLocation, pthread_self());
  ret = vsprintf(temp + num, m, p);

  if(ret < 0) {
    SNetUtilDebugFatal("Debug message too long!");
  }

  num += ret;

  ret = sprintf(temp + num, ")\n");

  if(ret < 0) {
    SNetUtilDebugFatal("Debug message too long!");
  }

  fputs(temp, stderr);
  fflush(stderr);
  va_end(p);
}
