#include <execinfo.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bool.h"
#include "debug.h"

#define snet_stream_desc_t void
#define snet_record_t void
#include "xworker.h"
extern struct worker* SNetThreadGetSelf(void);

void (*SNetTraceFunc)(const char *func);

/* max length of function names */
#define TRACE_FUNC_NAME_LEN     80

static int snet_trace_level;
static bool snet_trace_color;

/* Test whether output may be a terminal. */
static bool show_colors(void)
{
  struct stat st;
  fstat(1, &st);
  return S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode);
}


/* Add terminal color escape codes to a string
 * to make it appear in the color blue. */
static const char *blue(const char *s, char *out)
{
  if (snet_trace_color) {
    sprintf(out, "\033[1;34m%s\033[m", s);
    return out;
  }
  return s;
}


/* Show a function name and a small stacktrace. */
void SNetTraceCalls(const char *func)
{
  const int     max = 20;
  const int     skip = 2;
  void         *array[max];
  int           count;
  int           todo;
  int           indent;
  char          string[max * TRACE_FUNC_NAME_LEN];
  char        **syms;
  struct worker *self;
  char         *strptr = string;

  count = backtrace(array, max);
  todo = count - skip;
  indent = 2 * (todo - 3);
  if (todo > snet_trace_level) todo = snet_trace_level;
  syms = backtrace_symbols(array + skip, todo);
  self = SNetThreadGetSelf();
  sprintf(strptr, "%s %d:\n", blue(func, string), self ? self->id : 0);
  strptr += strlen(strptr);
  for (int i = 0; i < todo; ++i) {
    char *p = strrchr(syms[i], '/');
    p = (p ? (p + 1) : syms[i]);
    sprintf(strptr, "%*s%s\n", indent, "", p);
    strptr += strlen(strptr);
  }
  free(syms);
  fputs(string, stdout);
}

/* Trace only the function name. */
void SNetTraceSimple(const char *func)
{
  char           string[TRACE_FUNC_NAME_LEN];
  struct worker *self = SNetThreadGetSelf();

  printf("%s %d\n", blue(func, string), self ? self->id : 0);
}

/* Trace nothing. */
void SNetTraceEmpty(const char *func)
{
  (void) func;
}

/* Called by trace(x). */
void (*SNetTraceFunc)(const char *func) = SNetTraceEmpty;

/* Set tracing: -1 disables, >= 0 sets call stack tracing depth. */
void SNetEnableTracing(int trace_level)
{
  if (trace_level < 0) {
    SNetTraceFunc = SNetTraceEmpty;
  } else {
    snet_trace_level = trace_level;
    snet_trace_color = show_colors();
    SNetTraceFunc = (trace_level > 0) ? SNetTraceCalls : SNetTraceSimple;
  }
}

/* Force tracing of a function call at a specific depth. */
void SNetTrace(const char *func, int trace_level)
{
  int old_level = snet_trace_level;
  snet_trace_level = trace_level;
  SNetTraceCalls(func);
  snet_trace_level = old_level;
}

