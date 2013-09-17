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
  return root;
}

resource_t* res_cpuinfo_resource_init(void)
{
  static const char cpuinfo[] = "/proc/cpuinfo";
  FILE *fp = fopen(cpuinfo, "r");
  resource_t* root = NULL;

  if (fp) {
    root = res_parse_cpuinfo(fp);
    fclose(fp);
  }
  else {
    res_warn("Could not open %s: %s\n", cpuinfo, strerror(errno));
  }

  return root;
}

