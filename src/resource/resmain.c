#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "resdefs.h"

static const char* listen_addr;
static int         listen_port;
static bool        show_resource;
static bool        show_topology;
static intmap_t*   slaves;

static void usage(void)
{
  fprintf(stderr, "Usage: %s [ options ... ]\n", res_get_program_name());
  fprintf(stderr, "Options:\n");
  fprintf(stderr, " -c config  : Read configuration from file 'config'.\n");
  fprintf(stderr, " -l address : Listen on IP address 'address'.\n");
  fprintf(stderr, " -p port    : Listen on TCP port 'port'.\n");
  fprintf(stderr, " -r         : Give hardware locality resource information.\n");
  fprintf(stderr, " -t         : Give processor/cache topology information.\n");
  fprintf(stderr, " -d         : Enable debugging output.\n");
  fprintf(stderr, " -v         : Enable informational messages.\n");
  fprintf(stderr, " -V         : Give version information.\n");
  exit(1);
}

static void get_version(void)
{
  fprintf(stderr, "%s: hwloc is %s.\n", res_get_program_name(),
#if ENABLE_HWLOC
          "enabled"
#else
          "disabled"
#endif
          );

#if defined(__DATE__) && defined(__TIME__)
  fprintf(stderr, "%s: compiled on %s.\n", res_get_program_name(),
          __TIME__  " " __DATE__);
#endif

#if defined(__GNUC_PATCHLEVEL__)
  fprintf(stderr, "%s: compiled by GCC version %d.%d.%d.\n",
          res_get_program_name(), __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
  exit(0);
}

static bool get_listen_addr(const char *spec)
{
  if (listen_addr) {
    xfree((void *) listen_addr);
  }
  if (spec && *spec) {
    listen_addr = xstrdup(spec);
  } else {
    listen_addr = NULL;
  }
  return true;
}

static bool get_listen_port(const char *spec)
{
  int port = 0, num, len = 0;

  num = sscanf(spec, "%d%n", &port, &len);
  if (num >= 1 && port >= 1024 && port < 65535 && len == strlen(spec)) {
    listen_port = port;
    return true;
  } else {
    fprintf(stderr, "%s: Invalid port specification '%s'.\n", res_get_program_name(), spec);
    return false;
  }
}

static bool get_bool(const char *spec, bool *result)
{
  bool success = true;

  if (*spec && strchr("0nNfF", *spec)) *result = false;
  else if (*spec && strchr("1yYtT", *spec)) *result = true;
  else {
      fprintf(stderr, "%s: Invalid flag '%s'.\n", res_get_program_name(), spec);
      success = false;
  }
  return success;
}

static void get_config(const char *fname)
{
  FILE *fp = fopen(fname, "r");

  if (!fp) {
    res_pexit(fname);
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
        int num, len = 0;
        val[0] = '\0';
        num = sscanf(buf, " %s %c %s %n", key, &ch, val, &len);
        if (num >= 3 && ch == '=' && (num == 2 || len == strlen(buf))) {
          if (!strcmp(key, "listen")) {
            valid = get_listen_addr(val);
          }
          else if (!strcmp(key, "port")) {
            valid = get_listen_port(val);
          }
          else if (!strcmp(key, "verbose")) {
            bool flag = false;
            valid = get_bool(val, &flag);
            if (valid) res_set_verbose(flag);
          }
          else if (!strcmp(key, "debug")) {
            bool flag = false;
            valid = get_bool(val, &flag);
            if (valid) res_set_debug(flag);
          }
        }

        if (!valid && !strcmp(key, "slave") && ch == '=') {
          char* eq = strchr(buf, '=');
          while (*++eq && isspace((unsigned char) *eq)) { }
          if (3 <= strlen(eq)) {
            int max = 1;
            char* end = eq + strlen(eq);
            while (--end >= eq && isspace((unsigned char) *end)) { *end = '\0'; }
            if (slaves == NULL) {
              slaves = res_map_create();
            } else {
              max += res_map_max(slaves);
            }
            res_map_add(slaves, max, xstrdup(eq));
            valid = true;
          }
        }

        if (!valid) {
          fprintf(stderr, "%s: Invalid configuration at line %d of %s. (%d,%d)\n",
                  res_get_program_name(), line, fname, num, len);
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

static void get_options(int argc, char **argv)
{
  int c;

  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
  res_set_program_name(basename(*argv));
  listen_port = RES_DEFAULT_LISTEN_PORT;

  while ((c = getopt(argc, argv, "c:dhl:p:rtvV?")) != EOF) {
    switch (c) {
      case 'c': get_config(optarg); break;
      case 'd': res_set_debug(true); break;
      case 'l': if (!get_listen_addr(optarg)) exit(1); break;
      case 'p': if (!get_listen_port(optarg)) exit(1); break;
      case 'r': show_resource = true; break;
      case 't': show_topology = true; break;
      case 'v': res_set_verbose(true); break;
      case 'V': get_version(); break;
      default: usage();
    }
  }
}

int main(int argc, char **argv)
{
  get_options(argc, argv);

  if (res_topo_init() == false) {
    res_error("%s: Could not initialize hardware topology information.\n",
              *argv);
  } else {
    if (show_resource) {
      char *str = res_system_resource_string(LOCAL_HOST);
      fputs(str, stdout);
      xfree(str);
    }
    else if (show_topology) {
      char *str = res_system_host_string(LOCAL_HOST);
      fputs(str, stdout);
      xfree(str);
    }
    else {
      res_service(listen_addr, listen_port, slaves);
    }

    res_topo_destroy();
  }

  return 0;
}

