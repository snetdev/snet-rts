#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include "resproto.h"
#include "resdefs.h"

static const char* prog;
static int port;
static bool debug;
static bool verbose;

static void usage(void)
{
  fprintf(stderr, "Usage: %s [ options ... ]\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-c config: Read configuration from file 'config'.\n");
  fprintf(stderr, "\t-p port  : Use 'port' for incoming TCP connections.\n");
  fprintf(stderr, "\t-d       : Enable debugging information.\n");
  fprintf(stderr, "\t-v       : Enable informational messages.\n");
  exit(1);
}

void pexit(const char *mesg)
{
  fprintf(stderr, "%s: %s: %s\n", prog, mesg, strerror(errno));
  exit(1);
}

static bool getport(const char *spec)
{
  int p = 0;
  if (sscanf(spec, "%d", &p) == 1 && p >= 1024 && p < 65535) {
    port = p;
  } else {
    fprintf(stderr, "%s: Invalid port '%s'.\n", prog, spec);
  }
  return p == port;
}

static bool getbool(const char *spec, bool *result)
{
  bool success = true;
  switch (*spec) {
    case '1':
    case 'y':
    case 'Y':
    case 't':
    case 'T': *result = true; break;
    case '0':
    case 'n':
    case 'N':
    case 'f':
    case 'F': *result = false; break;
    default:
      fprintf(stderr, "%s: Invalid flag '%s'.\n", prog, spec);
      success = false;
  }
  return success;
}

static void config(const char *fname)
{
  FILE *fp = fopen(fname, "r");

  if (!fp) {
    pexit(fname);
  }
  else {
    int line = 0, errors = 0;
    char buf[PATH_MAX];

    while (fgets(buf, sizeof buf, fp)) {
      bool valid = false;
      char ch;
      ++line;
      if (sscanf(buf, " %c", &ch) > 0 && ch != '#') {
        char key[PATH_MAX], val[PATH_MAX];
        int len = 0;
        int n = sscanf(buf, " %s = %s %n", key, val, &len);
        if (n >= 2 && len == strlen(buf)) {
          if (!strcmp(key, "port")) {
            if (getport(val)) {
              valid = true;
            }
          }
          else if (!strcmp(key, "verbose")) {
            if (getbool(val, &verbose)) {
              valid = true;
            }
          }
          else if (!strcmp(key, "debug")) {
            if (getbool(val, &debug)) {
              valid = true;
            }
          }
        }
        if (!valid) {
          fprintf(stderr, "%s: Invalid configuration at line %d of %s.\n",
                  prog, line, fname);
          ++errors;
        }
      }
    }
    fclose(fp);
    if (errors) {
      exit(1);
    }
  }
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
  if (verbose) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
  }
}

void res_debug(const char *fmt, ...)
{
  if (debug) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
  }
}

static void options(int argc, char **argv)
{
  int c;

  prog = basename(*argv);
  port = RES_DEFAULT_LISTEN_PORT;

  while ((c = getopt(argc, argv, "c:dhp:v?")) != EOF) {
    switch (c) {
      case 'c': config(optarg); break;
      case 'd': debug = true; break;
      case 'p': if (!getport(optarg)) exit(1); break;
      case 'v': verbose = true; break;
      default: usage();
    }
  }
}

int main(int argc, char **argv)
{
  options(argc, argv);

  hwinit();

  return 0;
}

