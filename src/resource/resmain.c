#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>

#include "resdefs.h"

static const char* prog;
static int port;
static bool debug;
static bool verbose;

static void usage(void)
{
  fprintf(stderr, "Usage: %s [ options ... ]\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, " -c config: Read configuration from file 'config'.\n");
  fprintf(stderr, " -p port  : Use 'port' for incoming TCP connections.\n");
  fprintf(stderr, " -d       : Enable debugging information.\n");
  fprintf(stderr, " -v       : Enable informational messages.\n");
  exit(1);
}

void pexit(const char *mesg)
{
  fprintf(stderr, "%s: %s: %s\n", prog, mesg, strerror(errno));
  exit(1);
}

bool res_get_debug(void) { return debug; }
bool res_get_verbose(void) { return verbose; }

static bool get_port(const char *spec)
{
  int p = 0;
  if (sscanf(spec, "%d", &p) == 1 && p >= 1024 && p < 65535) {
    port = p;
  } else {
    fprintf(stderr, "%s: Invalid port '%s'.\n", prog, spec);
  }
  return p == port && p >= 1024 && p < 65535;
}

static bool get_bool(const char *spec, bool *result)
{
  bool success = true;

  if (*spec && strchr("0nNfF", *spec)) *result = false;
  else if (*spec && strchr("1yYtT", *spec)) *result = true;
  else {
      fprintf(stderr, "%s: Invalid flag '%s'.\n", prog, spec);
      success = false;
  }
  return success;
}

static void get_config(const char *fname)
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
            valid = get_port(val);
          }
          else if (!strcmp(key, "verbose")) {
            valid = get_bool(val, &verbose);
          }
          else if (!strcmp(key, "debug")) {
            valid = get_bool(val, &debug);
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

void res_assert_failed(const char *fn, int ln, const char *msg)
{
  fprintf(stderr, "%s: assertion failed:%s:%d: %s\n", prog, fn, ln, msg);
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
  if (verbose) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
  }
}

void res_debug(const char *fmt, ...)
{
  if (debug) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
  }
}

static void get_options(int argc, char **argv)
{
  int c;

  prog = basename(*argv);
  port = RES_DEFAULT_LISTEN_PORT;

  while ((c = getopt(argc, argv, "c:dhp:v?")) != EOF) {
    switch (c) {
      case 'c': get_config(optarg); break;
      case 'd': debug = true; break;
      case 'p': if (!get_port(optarg)) exit(1); break;
      case 'v': verbose = true; break;
      default: usage();
    }
  }
}

int main(int argc, char **argv)
{
  get_options(argc, argv);

  res_topo_init();

  res_service(port);

  res_topo_destroy();

  return 0;
}

