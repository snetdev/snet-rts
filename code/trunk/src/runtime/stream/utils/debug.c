#include <pthread.h>
#include <string.h>

#include "debug.h"
#include "record_p.h"
#include "memfun.h"
#include "distribution.h"

extern char* SNetUtilDebugDumpRecord(snet_record_t *source, char* storage) {
  if(source == NULL) {
    sprintf(storage, "(RECORD NONE)");
  } else {
    switch(SNetRecGetDescriptor(source)) {
      case REC_data:
        sprintf(storage, "(RECORD %p DATA)", 
              source);
      break;

      case REC_sync:
        sprintf(storage, "(RECORD %p SYNC (NEW INPUT %p))",
              source,
              SNetRecGetStream(source));
      break;

      case REC_collect:
        sprintf(storage, "(RECORD %p COLLECT (NEW STREAM %p))",
              source,
              SNetRecGetStream(source));
      break;

      case REC_sort_end:
        sprintf(storage, "(RECORD %p SORTEND (LEVEL %d) (NUM %d))",
              source,
              SNetRecGetLevel(source),
              SNetRecGetNum(source));
      break;

      case REC_terminate:
        sprintf(storage, "(RECORD %p TERMINATE)",
              source);
      break;

      case REC_trigger_initialiser:
        sprintf(storage, "(RECORD %p TRIGGER_INITIALISER)",
              source);
      break;
    }
  }
  return storage;
}

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
