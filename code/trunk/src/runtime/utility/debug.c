#include "debug.h"
#include "pthread.h"
#include "record.h"

extern char* SNetUtilDebugDumpRecord(snet_record_t *source, char* storage) {
  if(source == NULL) {
    sprintf(storage, "(RECORD NONE)");
  } else {
    switch(SNetRecGetDescriptor(source)) {
      case REC_data:
        sprintf(storage, "(RECORD %x DATA)", 
              (unsigned int) source);
      break;

      case REC_sync:
        sprintf(storage, "(RECORD %x SYNC (NEW INPUT %x))",
              (unsigned int) source,
              (unsigned int) SNetRecGetStream(source));
      break;

      case REC_collect:
        sprintf(storage, "(RECORD %x COLLECT (NEW STREAM %x))",
              (unsigned int) source,
              (unsigned int) SNetRecGetStream(source));
      break;

      case REC_sort_begin:
        sprintf(storage, "(RECORD %x SORTBEGIN (LEVEL %d) (NUM %d))",
              (unsigned int) source,
              SNetRecGetLevel(source),
              SNetRecGetNum(source));
      break;

      case REC_sort_end:
        sprintf(storage, "(RECORD %x SORTEND (LEVEL %d) (NUM %d))",
              (unsigned int) source,
              SNetRecGetLevel(source),
              SNetRecGetNum(source));
      break;

      case REC_terminate:
        sprintf(storage, "(RECORD %x TERMINATE)",
              (unsigned int) source);
      break;

      case REC_probe:
        sprintf(storage, "(RECORD %x PROBE)",
              (unsigned int) source);
      break;
    }
  }
  return storage;
}

extern void SNetUtilDebugFatal(char* m, ...) {
  va_list p;

  va_start(p, m);
  fprintf(stderr, "(SNET FATAL (THREAD %x) ", (unsigned int) pthread_self());
  vfprintf(stderr, m, p);
  fputs(")\n\n", stderr);
  abort();
}

extern void SNetUtilDebugNotice(char *m, ...) {
  va_list p;

  va_start(p, m);
  fprintf(stderr, "(SNET NOTICE (THREAD %x) ", (unsigned int) pthread_self());
  vfprintf(stderr, m, p);
  fputs(")\n", stderr);
  fflush(stderr);
}
