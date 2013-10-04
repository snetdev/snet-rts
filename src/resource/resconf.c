#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <unistd.h>

#include "resdefs.h"
#include "resconf.h"

typedef struct res_client_conf res_client_conf_t;
typedef struct res_server_conf res_server_conf_t;

bool res_get_listen_addr(const char *spec, const char **listen_addr)
{
  if (listen_addr) {
    xfree((void *) *listen_addr);
  }
  if (spec && *spec) {
    *listen_addr = xstrdup(spec);
  } else {
    *listen_addr = NULL;
  }
  return true;
}

bool res_get_listen_port(const char *spec, int *listen_port)
{
  int port = 0, num, len = 0;

  num = sscanf(spec, "%d%n", &port, &len);
  if (num >= 1 && port >= 1024 && port < 65535 && len == strlen(spec)) {
    *listen_port = port;
    return true;
  } else {
    res_warn("%s: Invalid port spec '%s'.\n", res_get_program_name(), spec);
    return false;
  }
}

bool res_get_bool_conf(const char *spec, bool *result)
{
  bool success = true;

  if (*spec && strchr("0nNfF", *spec)) *result = false;
  else if (*spec && strchr("1yYtT", *spec)) *result = true;
  else {
      res_warn("%s: Invalid flag '%s'.\n", res_get_program_name(), spec);
      success = false;
  }
  return success;
}

void res_init_server_conf(res_server_conf_t *conf)
{
  memset(conf, 0, sizeof(*conf));
  conf->slaves = NULL;
  conf->listen_addr = NULL;
  conf->listen_port = RES_DEFAULT_LISTEN_PORT;
}

void res_init_client_conf(res_client_conf_t *conf)
{
  memset(conf, 0, sizeof(*conf));
  conf->server_addr = NULL;
  conf->server_port = RES_DEFAULT_LISTEN_PORT;
}

void res_get_server_config(const char *fname, res_server_conf_t *conf)
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
      char ch = '\0';
      ++line;
      /* Only examine non-comment lines. */
      if (sscanf(buf, " %c", &ch) > 0 && ch != '#') {
        char key[PATH_MAX], val[PATH_MAX];
        int num, len = 0;
        val[0] = '\0';
        num = sscanf(buf, " %s %c %s %n", key, &ch, val, &len);
        if (num >= 3 && ch == '=' && (num == 2 || len == strlen(buf))) {
          if (!strcmp(key, "listen")) {
            valid = res_get_listen_addr(val, &conf->listen_addr);
          }
          else if (!strcmp(key, "port")) {
            valid = res_get_listen_port(val, &conf->listen_port);
          }
          else if (!strcmp(key, "verbose")) {
            valid = res_get_bool_conf(val, &conf->verbose);
          }
          else if (!strcmp(key, "debug")) {
            valid = res_get_bool_conf(val, &conf->debug);
          }
        }

        if (!valid && !strcmp(key, "slave") && ch == '=') {
          char* eq = strchr(buf, '=');
          while (*++eq && isspace((unsigned char) *eq)) { }
          if (3 <= strlen(eq)) {
            int max = 1;
            char* end = eq + strlen(eq);
            while (--end >= eq && isspace((unsigned char) *end)) { *end = '\0'; }
            if (conf->slaves == NULL) {
              conf->slaves = res_map_create();
            } else {
              max += res_map_max(conf->slaves);
            }
            res_map_add(conf->slaves, max, xstrdup(eq));
            valid = true;
          }
        }

        if (!valid) {
          res_warn("%s: Invalid configuration at line %d of %s. (%d,%d)\n",
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

void res_get_client_config(const char *arg, res_client_conf_t *conf)
{
  if (access(arg, R_OK) == 0) {
    FILE *fp = fopen(arg, "r");

    if (!fp) {
      res_pexit(arg);
    }
    else {
      int line = 0, errors = 0;
      char buf[PATH_MAX];

      while (fgets(buf, sizeof buf, fp)) {
        bool valid = false;
        char ch = '\0';
        ++line;
        /* Only examine non-comment lines. */
        if (sscanf(buf, " %c", &ch) > 0 && ch != '#') {
          char key[PATH_MAX], val[PATH_MAX];
          int num, len = 0;
          val[0] = '\0';
          num = sscanf(buf, " %s %c %s %n", key, &ch, val, &len);
          if (num >= 3 && ch == '=' && (num == 2 || len == strlen(buf))) {
            if (!strcmp(key, "listen")) {
              valid = res_get_listen_addr(val, &conf->server_addr);
            }
            else if (!strcmp(key, "port")) {
              valid = res_get_listen_port(val, &conf->server_port);
            }
          }

          if (!valid) {
            res_warn("%s: Invalid configuration at line %d of %s. (%d,%d)\n",
                    res_get_program_name(), line, arg, num, len);
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
  else {
    bool valid = false;
    const char* colon = strchr(arg, ':');

    if (colon) {
      if (isdigit((unsigned char) colon[1])) {
        char* copy = xstrdup(arg);
        char* save = NULL;
        char* addr = strtok_r(copy, ":", &save);
        char* srvc = strtok_r(NULL, "", &save);
        int port = atoi(srvc);

        conf->server_addr = xstrdup(addr);
        conf->server_port = port;
        xfree(copy);
        valid = true;
      }
    }
    if (!valid) {
      res_error("%s: Invalid server configuration specification: '%s'.\n",
                res_get_program_name(), arg);
    }
  }
}

