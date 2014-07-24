#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include "resdefs.h"

typedef enum res_kind res_kind_t;
typedef struct resource resource_t;

#include "resource.h"

/*
 * Convert a hierarchical resource topology to a single string.
 * Input parameters:
 *      obj: the resource and its children to convert to string.
 *      str: the destination string where output is appended to.
 *      len: the current length of the output string.
 *      size: the current size of the destination string.
 * Output result: Return the current string and its size.
 * When the string memory is too small then the string must be
 * dynamically reallocated to accomodate the new output.
 * The new size of the string must be communicated via the size pointer.
 */
char* res_resource_object_string(resource_t* obj, char* str, int len, int *size)
{
  assert(obj);
  assert(str);
  assert(*size >= 100);
  assert(len >= 0 && len < *size);
  assert(str[len] == '\0');

  if (*size - len < 1024) {
    str = xrealloc(str, *size + 1024);
    *size += 1024;
  }
  snprintf(str + len, *size - len,
           "%*s{ kind %s depth %d logical %d ",
           (obj->depth + 1) * 2, "",
           res_kind_string(obj->kind), obj->depth, obj->logical);
  len += strlen(str + len);

  if (obj->kind == Proc) {
    snprintf(str + len, *size - len, "physical %d ", obj->physical);
    len += strlen(str + len);
  }
  if (obj->kind == Cache) {
    snprintf(str + len, *size - len, "level %d ", obj->cache_level);
    len += strlen(str + len);
  }
  if (obj->num_children > 0) {
    int i;
    snprintf(str + len, *size - len, "children %d \n", obj->num_children);
    len += strlen(str + len);
    for (i = 0; i < obj->num_children; ++i) {
      str = res_resource_object_string(obj->children[i], str, len, size);
      len += strlen(str + len);
    }
    snprintf(str + len, *size - len,
             "%*s} \n", (obj->depth + 1) * 2, "");
  } else {
    snprintf(str + len, *size - len, "} \n");
  }

  return str;
}

void res_relate_resources(resource_t *parent, resource_t *child)
{
  if (parent->num_children == parent->max_children) {
    int max = MAX(1, 2 * parent->max_children);
    parent->max_children = max;
    parent->children = xrealloc(parent->children, max * sizeof(resource_t *));
  }
  assert(parent->num_children < parent->max_children);
  parent->children[parent->num_children++] = child;
}

resource_t* res_add_resource(
  resource_t *parent,
  res_kind_t kind,
  int topo_depth,
  int logical_index,
  int physical_index,
  int cache_level)
{
  resource_t *res = xnew(resource_t);
  res->depth = topo_depth;
  res->kind = kind;
  res->logical = logical_index;
  res->physical = physical_index;
  res->cache_level = cache_level;
  res->parent = parent;
  res->children = 0;
  res->num_children = 0;
  res->max_children = 0;
  res->first_core = 0;
  res->last_core = 0;
  res->first_proc = 0;
  res->last_proc = 0;
  if (parent) {
    res_relate_resources(parent, res);
  }
  return res;
}

resource_t* res_resource_init(void)
{
  resource_t    *local_root = NULL;
  const char    *env = getenv("RESSERV");

  if (!env || strstr(env, "hwloc")) {
    local_root = res_hwloc_resource_init();
  }

  if (local_root == NULL && (!env || strstr(env, "cpuinfo"))) {
    if (env && strstr(env, "cpuinfo") && res_get_verbose()) {
      res_warn("Using cpuinfo resources.\n");
    }
    local_root = res_cpuinfo_resource_init();
  }

  return local_root;
}

void res_resource_free(resource_t* res)
{
  if (res) {
    if (res->children) {
      int i;
      for (i = 0; i < res->num_children; ++i) {
        res_resource_free(res->children[i]);
      }
      xfree(res->children);
    }
    xfree(res);
  }
}

/* Convert the kind of resource to a static string. */
const char *res_kind_string(int kind)
{
  return
#define NAME(x) #x
#define KIND(k) kind == k ? NAME(k) :
  KIND(System)
  KIND(Machine)
  KIND(Node)
  KIND(Socket)
  KIND(Cache)
  KIND(Core)
  KIND(Proc)
  KIND(Other)
  "Unknown";
#undef NAME
#undef KIND
}

