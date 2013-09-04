#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include "resdefs.h"

typedef enum res_kind res_kind_t;
typedef struct resource resource_t;

#include "resource.h"

int res_local_cores(void)
{
  resource_t *res_root = res_local_root();
  int count = res_root->last_core - res_root->first_core + 1;
  assert(count > 0);
  return count;
}

int res_local_procs(void)
{
  resource_t *res_root = res_local_root();
  int count = res_root->last_proc - res_root->first_proc + 1;
  assert(count > 0);
  return count;
}

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
char* res_resource_string(resource_t* obj, char* str, int len, int *size)
{
  if (!obj) {
    obj= res_local_root();
  }
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
      str = res_resource_string(obj->children[i], str, len, size);
      len += strlen(str + len);
    }
    snprintf(str + len, *size - len,
             "%*s} \n", (obj->depth + 1) * 2, "");
  } else {
    snprintf(str + len, *size - len, "} \n");
  }

  return str;
}

static void res_relate_resources(resource_t *parent, resource_t *child)
{
  if (parent->num_children == parent->max_children) {
    int max = MAX(1, 2 * parent->max_children);
    parent->max_children = max;
    parent->children = xrealloc(parent->children, max * sizeof(resource_t *));
  }
  assert(parent->num_children < parent->max_children);
  parent->children[parent->num_children++] = child;
}

static resource_t* res_add_resource(
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

/* Convert HWLOC type to resource kind. */
static res_kind_t res_type_to_kind(hwloc_obj_type_t type)
{
  res_kind_t kind = Other;
  switch (type) {
    case HWLOC_OBJ_SYSTEM:  kind = System;  break;
    case HWLOC_OBJ_MACHINE: kind = Machine; break;
    case HWLOC_OBJ_NODE:    kind = Node;    break;
    case HWLOC_OBJ_SOCKET:  kind = Socket;  break;
    case HWLOC_OBJ_CACHE:   kind = Cache;   break;
    case HWLOC_OBJ_CORE:    kind = Core;    break;
    case HWLOC_OBJ_PU:      kind = Proc;    break;
    default:
      res_error("Invalid type %s\n", hwloc_obj_type_string(type));
  }
  return kind;
}

static resource_t* traverse_resources(
  hwloc_topology_t topo,
  hwloc_obj_t obj,
  int depth,
  resource_t* parent)
{
  int           i, cache_depth;
  res_kind_t    kind;
  resource_t*   res;

  if (obj->logical_index > MAX_LOGICAL) {
    res_warn("Ignoring hwloc object %s at depth %d with logical %d.\n",
              hwloc_obj_type_string(obj->type), depth, obj->logical_index);
    return NULL;
  }

  switch (obj->type) {
    case HWLOC_OBJ_SYSTEM:
    case HWLOC_OBJ_MACHINE:
    case HWLOC_OBJ_NODE:
    case HWLOC_OBJ_SOCKET:
    case HWLOC_OBJ_CACHE:
    case HWLOC_OBJ_CORE:
    case HWLOC_OBJ_PU:
      kind = res_type_to_kind(obj->type);
      break;
    default:
      res_warn("Ignoring hwloc object %s at depth %d.\n",
                hwloc_obj_type_string(obj->type), depth);
      return NULL;
  }

  cache_depth = ((kind == Cache && obj->attr) ? obj->attr->cache.depth : 0);
  res = res_add_resource(parent, kind, depth,
                         obj->logical_index, obj->os_index, cache_depth);

  if (res->kind == Proc) {
    assert(obj->arity == 0);
    res->first_proc = res->last_proc = obj->logical_index;
    res->first_core = res->last_core = parent->first_core;
  }
  else {
    if (res->kind == Core) {
      res->first_core = res->last_core = obj->logical_index;
    }
    else if (res->kind > Core) {
      res->first_core = res->last_core = parent->first_core;
    }

    for (i = 0; i < obj->arity; i++) {
      if (i == 0 || obj->children[i]->type >= HWLOC_OBJ_NODE) {
        traverse_resources(topo, obj->children[i], depth + 1, res);
      }
    }

    for (i = 0; i < res->num_children; i++) {
      if (i == 0) {
        res->first_proc = res->children[i]->first_proc;
        res->last_proc = res->children[i]->last_proc;
      } else {
        if (res->children[i]->first_proc != res->last_proc + 1) {
          res_warn("Proc gap at depth %d, child %d, %d -> %d\n",
                   depth, i, res->last_proc, res->children[i]->first_proc);
        }
        res->last_proc = res->children[i]->last_proc;
      }

      if (res->kind < Core) {
        if (i == 0) {
          res->first_core = res->children[i]->first_core;
          res->last_core = res->children[i]->last_core;
        } else {
          res->last_core = res->children[i]->last_core;
        }
      }
    }
  }
  res_info("Depth %d, %-7s, cores %d - %d, procs %d - %d\n",
           depth, res_kind_string(res->kind),
           res->first_core, res->last_core,
           res->first_proc, res->last_proc);

  return res;
}

resource_t* res_resource_init(void)
{
  hwloc_topology_t      hwloc_topo;
  hwloc_obj_t           hwloc_root;
  resource_t           *local_root;

  hwloc_topology_init(&hwloc_topo);
  hwloc_topology_load(hwloc_topo);
  hwloc_root = hwloc_get_root_obj(hwloc_topo);
  local_root = traverse_resources(hwloc_topo, hwloc_root, DEPTH_ZERO, NO_PARENT);
  hwloc_topology_destroy(hwloc_topo);
  return local_root;
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

