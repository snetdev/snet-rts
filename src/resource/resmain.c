#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "resdefs.h"
#include "resconf.h"

static bool        show_resource;
static bool        show_topology;

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

static void get_config(const char *fname, res_server_conf_t *conf)
{
  res_init_server_conf(conf);
  res_get_server_config(fname, conf);
}

static void get_options(int argc, char **argv, res_server_conf_t *conf)
{
  int c;

  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
  res_set_program_name(basename(*argv));
  res_init_server_conf(conf);

  while ((c = getopt(argc, argv, "c:dhl:p:rtvV?")) != EOF) {
    switch (c) {
      case 'c': get_config(optarg, conf); break;
      case 'd': conf->debug = true; break;
      case 'l': if (!res_get_listen_addr(optarg, &conf->listen_addr)) {
                  exit(1); 
                } break;
      case 'p': if (!res_get_listen_port(optarg, &conf->listen_port)) {
                  exit(1);
                } break;
      case 'r': show_resource = true; break;
      case 't': show_topology = true; break;
      case 'v': conf->verbose = true; break;
      case 'V': get_version(); break;
      default: usage();
    }
  }
  res_set_debug(conf->debug);
  res_set_verbose(conf->verbose);
}

int main(int argc, char **argv)
{
  res_server_conf_t  config;

  get_options(argc, argv, &config);

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
      res_service(config.listen_addr, config.listen_port, config.slaves);
    }

    res_topo_destroy();
  }

  return 0;
}

