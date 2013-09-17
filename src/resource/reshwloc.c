#include <hwloc.h>
#include "resdefs.h"
#include "resource.h"

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
  res_debug("Depth %d, %-7s, cores %d - %d, procs %d - %d\n",
            depth, res_kind_string(res->kind),
            res->first_core, res->last_core,
            res->first_proc, res->last_proc);

  return res;
}

resource_t* res_hwloc_resource_init(void)
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

