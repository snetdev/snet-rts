#include "debug.h"

extern void SNetUtilDebugFatal(char* m, ...) {
  va_list p;

  va_start(p, m);
  fprintf(stderr, "\n\n ** FATAL ERROR ** ");
  vfprintf(stderr, m, p);
  fputs("\n\n", stderr);
  exit(-1);
}

extern void SNetUtilDebugNotice(char *m, ...) {
  va_list p;

  va_start(p, m);
  fprintf(stderr, "\n\n ** RUNTIME NOTICE ** ");
  vfprintf(stderr, m, p);
  fputs("\n", stderr);
  fflush(stderr);
}
