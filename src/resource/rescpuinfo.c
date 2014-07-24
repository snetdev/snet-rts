#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/param.h>
#include "resdefs.h"
#include "resource.h"

static int EQ(const char* key, const char* str)
{
  return (*key == *str) && !strncmp(key, str, strlen(str));
}

static void res_fix_logical(resource_t* root)
{
  int s, c, p, num_procs = 0, num_cores = 0, num_socks = 0;
  for (s = 0; s < root->num_children; ++s) {
    resource_t* sock = root->children[s];
    sock->logical = num_socks++;
    for (c = 0; c < sock->num_children; ++c) {
      resource_t* core = sock->children[c];
      core->logical = num_cores++;
      core->first_core = core->last_core = core->logical;
      for (p = 0; p < core->num_children; ++p) {
        resource_t* proc = core->children[p];
        proc->logical = num_procs++;
        proc->first_proc = proc->last_proc = proc->logical;
        proc->first_core = proc->last_core = core->logical;
      }
      core->first_proc = core->children[0]->logical;
      core->last_proc = core->children[core->num_children - 1]->logical;
    }
    sock->first_proc = sock->children[0]->first_proc;
    sock->first_core = sock->children[0]->first_core;
    sock->last_proc = sock->children[sock->num_children - 1]->last_proc;
    sock->last_core = sock->children[sock->num_children - 1]->last_core;
  }
  root->first_proc = root->children[0]->first_proc;
  root->first_core = root->children[0]->first_core;
  root->last_proc = root->children[root->num_children - 1]->last_proc;
  root->last_core = root->children[root->num_children - 1]->last_core;
}

static resource_t* res_parse_cpuinfo(FILE *fp)
{
  struct cpuinfo {
    int processor;
    int physical;
    int siblings;
    int core_id;
    int num_cores;
  } info;
  char buf[BUFSIZ];
  resource_t *root = NULL;
  int i;

  memset(&info, 0, sizeof(info));

  while (fgets(buf, sizeof buf, fp)) {
    if (buf[0] == '\n') {
      resource_t *socket = NULL, *core = NULL, *proc = NULL;

      if (!root) {
        root = res_add_resource(NULL, Machine, 0, 0, 0, 0);
      }
      for (i = 0; i < root->num_children; ++i) {
        if (root->children[i]->physical == info.physical) {
          socket = root->children[i];
          break;
        }
      }
      if (socket == NULL) {
        socket = res_add_resource(root, Socket, 1, info.physical, info.physical, 0);
      }
      for (i = 0; i < socket->num_children; ++i) {
        if (socket->children[i]->physical == info.core_id) {
          core = socket->children[i];
          break;
        }
      }
      if (core == NULL) {
        core = res_add_resource(socket, Core, 2, info.core_id, info.core_id, 0);
      }
      for (i = 0; i < core->num_children; ++i) {
        if (core->children[i]->physical == info.processor) {
          proc = core->children[i];
          break;
        }
      }
      if (proc) {
        res_warn("%s: Duplicate processor %d\n", __func__, info.processor);
      } else {
        proc = res_add_resource(core, Proc, 3, info.processor, info.processor, 0);
      }
    } else {
      char* save = NULL;
      char* key = strtok_r(buf, ":", &save);
      char* val = strtok_r(NULL, "", &save);
      int* dest = NULL;

      if (EQ(key, "processor")) dest = &info.processor;
      else if (EQ(key, "physical id")) dest = &info.physical;
      else if (EQ(key, "siblings")) dest = &info.siblings;
      else if (EQ(key, "core id")) dest = &info.core_id;
      else if (EQ(key, "cpu cores")) dest = &info.num_cores;
      if (dest) {
        sscanf(val, " %d", dest);
      }
    }
  }
  if (root) {
    res_fix_logical(root);
  }
  return root;
}

resource_t* res_cpuinfo_resource_init(void)
{
  static const char cpuinfo[] = "/proc/cpuinfo";
  const char* env = getenv("CPUINFOFILE");
  const char* infile = env ? env : cpuinfo;
  FILE *fp = fopen(infile, "r");
  resource_t* root = NULL;

  if (fp) {
    if (env && res_get_verbose()) {
      res_warn("Using cpuinfofile %s.\n", env);
    }
    root = res_parse_cpuinfo(fp);
    fclose(fp);
  } else {
    res_warn("Could not open file %s: %s.\n", infile, strerror(errno));
  }

  return root;
}

