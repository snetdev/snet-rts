#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "resdefs.h"

static const char* res_program_name;
static bool        res_opt_debug;
static bool        res_opt_verbose;

const char* res_get_program_name(void)
{
  return res_program_name ? res_program_name : "";
}

void res_set_program_name(const char* prog) { res_program_name = prog; }
bool res_get_debug(void) { return res_opt_debug; }
void res_set_debug(bool flag) { res_opt_debug = flag; }
bool res_get_verbose(void) { return res_opt_verbose; }
void res_set_verbose(bool flag) { res_opt_verbose = flag; }

void res_pexit(const char *mesg)
{
  fprintf(stderr, "%s: %s: %s\n",
          res_get_program_name(), mesg, strerror(errno));
  exit(1);
}

void res_assert_failed(const char *fn, int ln, const char *msg)
{
  fprintf(stderr, "%s: assertion failed:%s:%d: %s\n",
          res_get_program_name(), fn, ln, msg);
  exit(1);
}

void res_warn(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void res_error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(1);
}

void res_info(const char *fmt, ...)
{
  if (res_get_verbose()) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
  }
}

void res_debug(const char *fmt, ...)
{
  if (res_get_debug()) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
  }
}

