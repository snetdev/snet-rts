#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include "resproto.h"

#define RES_DEFAULT_LISTEN_PORT 56389

static const char* prog;
static int port;

static void usage(void)
{
  fprintf(stderr, "Usage: %s [ options ... ]\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-c config: read configuration from file 'config'.\n");
  exit(1);
}

static void pexit(const char *mesg)
{
  perror(mesg);
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

static void config(const char *fname)
{
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    pexit(fname);
  } else {
    char buf[PATH_MAX];
    int line = 0;
    while (fgets(buf, sizeof buf, fp)) {
      bool valid = false;
      char ch;
      ++line;
      if (sscanf(buf, " %c", &ch) > 0 && ch != '#') {
        char key[PATH_MAX], val[PATH_MAX];
        int len = 0;
        int n = sscanf(buf, " %s = %s%n", key, val, &len);
        if (n >= 2) {
          if (!strcmp(key, "port")) {
            if (getport(val)) {
              valid = true;
            }
          }
        }
      }
      if (!valid) {
        fprintf(stderr, "%s: Invalid configuration at line %d of %s.\n",
                prog, line, fname);
      }
    }
    fclose(fp);
  }
}

static void options(int argc, char **argv)
{
  int c;

  prog = basename(*argv);
  port = RES_DEFAULT_LISTEN_PORT;

  while ((c = getopt(argc, argv, "c:h?")) != EOF) {
    switch (c) {
      case 'c': config(optarg); break;
      case 'p': if (!getport(optarg)) exit(1); break;
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

