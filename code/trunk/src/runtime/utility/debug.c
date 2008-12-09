#include "debug.h"
#include "pthread.h"
#include "record.h"

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

      case REC_sort_begin:
        sprintf(storage, "(RECORD %p SORTBEGIN (LEVEL %d) (NUM %d))",
              source,
              SNetRecGetLevel(source),
              SNetRecGetNum(source));
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

      case REC_probe:
        sprintf(storage, "(RECORD %p PROBE)",
              source);
      break;
#ifdef DISTRIBUTED_SNET
    case REC_route_update:
      sprintf(storage, "(RECORD %p ROUTE_UPDATE)",
              source);
      break;
    case REC_route_redirect:
      sprintf(storage, "(RECORD %p ROUTE_REDIRECT)",
	      source);
      break;
#endif /* DISTRIBUTED_SNET */
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
  va_end(p);
  abort();
}

extern void SNetUtilDebugNotice(char *m, ...) {
  va_list p;

  va_start(p, m);
  fprintf(stderr, "(SNET NOTICE (THREAD %x) ", (unsigned int) pthread_self());
  vfprintf(stderr, m, p);
  fputs(")\n", stderr);
  fflush(stderr);
  va_end(p);
}
