#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include "resdefs.h"

#define LOCAL_HOST      0
#define DEPTH_ZERO      0

typedef enum res_kind res_kind_t;
typedef struct resource resource_t;
typedef struct proc proc_t;
typedef struct core core_t;
typedef struct cache cache_t;
typedef struct numa numa_t;
typedef struct host host_t;
typedef struct topo topo_t;

#include "restopo.h"

static topo_t *global_topo;

topo_t* res_topo(void)
{
  return global_topo;
}

resource_t* res_local_root(void)
{
  return global_topo->host[LOCAL_HOST]->root;
}

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
char* res_topo_string(resource_t* obj, char* str, int len, int *size)
{
  if (!obj) {
    resource_t *res_root = res_local_root();
    obj = res_root;
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
      str = res_topo_string(obj->children[i], str, len, size);
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

static resource_t* traverse_topo(
  hwloc_topology_t topo,
  hwloc_obj_t obj,
  int depth,
  resource_t* parent)
{
  int           i, cache_depth;
  res_kind_t    kind;
  resource_t*   res;

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
        traverse_topo(topo, obj->children[i], depth + 1, res);
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

static proc_t* res_proc_create(resource_t* res)
{
  proc_t* proc = xnew(proc_t);
  proc->res = res;
  proc->assigned = 0;
  proc->client = NULL;
  assert(proc->res->kind == Proc);
  return proc;
}

static void res_core_collect(core_t* core, resource_t* res)
{
  int   i;

  switch (res->kind) {

    case Proc:
      core->nprocs += 1;
      core->procs = xrealloc(core->procs, core->nprocs * sizeof(proc_t *));
      core->procs[core->nprocs - 1] = res_proc_create(res);
      core->procs[core->nprocs - 1]->core = core;
      break;

    default:
      for (i = 0; i < res->num_children; ++i) {
        res_core_collect(core, res->children[i]);
      }
  }
}

static core_t* res_core_create(resource_t* res)
{
  core_t* core = xnew(core_t);
  core->res = res;
  core->hyper = 0;
  core->nprocs = 0;
  core->procs = NULL;
  core->assigned = 0;
  assert(core->res->kind >= Core);
  res_core_collect(core, res);
  if (core->nprocs >= 2) {
    core->hyper = core->nprocs;
  }
  assert(core->nprocs >= 1);
  return core;
}

static void res_cache_collect(cache_t* cache, resource_t* res)
{
  int   i;

  switch (res->kind) {

    case Core:
    case Proc:
      cache->ncores += 1;
      cache->cores = xrealloc(cache->cores, cache->ncores * sizeof(core_t *));
      cache->cores[cache->ncores - 1] = res_core_create(res);
      cache->cores[cache->ncores - 1]->cache = cache;
      cache->nprocs += cache->cores[cache->ncores - 1]->nprocs;
      break;

    default:
      for (i = 0; i < res->num_children; ++i) {
        res_cache_collect(cache, res->children[i]);
      }
  }
}

static cache_t* res_cache_create(resource_t* res)
{
  cache_t* cache = xnew(cache_t);
  cache->res = res;
  cache->ncores = 0;
  cache->cores = NULL;
  cache->nprocs = 0;
  cache->assigned = 0;
  res_cache_collect(cache, res);
  assert(cache->ncores >= 1);
  return cache;
}

static void res_numa_collect(numa_t* numa, resource_t* res)
{
  int   i;

  switch (res->kind) {

    case Cache:
    case Core:
      numa->ncaches += 1;
      numa->caches = xrealloc(numa->caches, numa->ncaches * sizeof(cache_t *));
      numa->caches[numa->ncaches - 1] = res_cache_create(res);
      numa->caches[numa->ncaches - 1]->numa = numa;
      numa->nprocs += numa->caches[numa->ncaches - 1]->nprocs;
      numa->ncores += numa->caches[numa->ncaches - 1]->ncores;
      break;

    default:
      for (i = 0; i < res->num_children; ++i) {
        res_numa_collect(numa, res->children[i]);
      }
  }
}

static numa_t* res_numa_create(resource_t* res)
{
  numa_t* numa = xnew(numa_t);
  numa->res = res;
  numa->ncaches = 0;
  numa->caches = NULL;
  numa->ncores = 0;
  numa->nprocs = 0;
  numa->assigned = 0;
  res_numa_collect(numa, res);
  assert(numa->ncaches >= 1);
  return numa;
}

static void res_host_collect(host_t* host, resource_t* res)
{
  int   i;

  switch (res->kind) {

    case System:
    case Machine:
      for (i = 0; i < res->num_children; ++i) {
        res_host_collect(host, res->children[i]);
      }
      break;

    default:
      host->size += 1;
      host->numa = xrealloc(host->numa, host->size * sizeof(numa_t *));
      host->numa[host->size - 1] = res_numa_create(res);
      host->numa[host->size - 1]->host = host;
      host->ncores += host->numa[host->size - 1]->ncores;
      host->nprocs += host->numa[host->size - 1]->nprocs;
  }
}

host_t* res_host_create(char* hostname, int index, resource_t* root)
{
  host_t* host = xnew(host_t);
  host->name = hostname;
  host->root = root;
  host->index = index;
  host->size = 0;
  host->numa = NULL;
  host->ncores = 0;
  host->nprocs = 0;
  host->cores = NULL;
  host->procs = NULL;
  host->assigned = 0;

  res_host_collect(host, root);

  if (res_get_debug()) {
    res_host_dump(host);
  }

  return host;
}

void res_host_destroy(host_t* host)
{
  int n, a, o, p;

  for (n = 0; n < host->size; ++n) {
    numa_t* numa = host->numa[n];
    for (a = 0; a < numa->ncaches; ++a) {
      cache_t* cache = numa->caches[a];
      for (o = 0; o < numa->ncores; ++o) {
        core_t* core = cache->cores[o];
        for (p = 0; p < core->nprocs; ++p) {
          proc_t* proc = core->procs[p];
          xfree(proc);
        }
        xfree(core->procs);
        xfree(core);
      }
      xfree(cache->cores);
      xfree(cache);
    }
    xfree(numa->caches);
    xfree(numa);
  }
  xfree(host->name);
  xfree(host->numa);
  xfree(host);
}

void res_topo_create(void)
{
  global_topo = xnew(topo_t);
  global_topo->size = 0;
  global_topo->host = NULL;
}

void res_topo_add_host(host_t *host, int id)
{
  int i, max = MAX(id + 1, global_topo->size);
  assert(id >= 0);
  assert(id >= global_topo->size || global_topo->host[id] == NULL);
  if (max > global_topo->size) {
    global_topo->host = xrealloc(global_topo->host, max * sizeof(host_t *));
    for (i = global_topo->size; i < max; ++i) {
      global_topo->host[i] = NULL;
    }
    global_topo->size = max;
  }
  global_topo->host[id] = host;
}

void res_host_dump(host_t* host)
{
  int n, a, o, p;

  printf("host %d, name %s, %d numas, %d cores, %d procs\n",
         host->index, host->name, host->size, host->ncores, host->nprocs);
  for (n = 0; n < host->size; ++n) {
    numa_t* numa = host->numa[n];
    printf("    numa %d, %d caches, %d cores, %d procs\n",
           n, numa->ncaches, numa->ncores, numa->nprocs);
    for (a = 0; a < numa->ncaches; ++a) {
      cache_t* cache = numa->caches[a];
      printf("        cache %d, %d cores, %d procs\n",
             a, numa->ncores, numa->nprocs);
      for (o = 0; o < cache->ncores; ++o) {
        core_t* core = cache->cores[o];
        printf("            core %d, %d procs, hyper %d\n",
               o, core->nprocs, core->hyper);
        for (p = 0; p < core->nprocs; ++p) {
          proc_t* proc = core->procs[p];
          printf("                proc %d, id %d\n", p, proc->res->first_proc);
        }
      }
    }
  }
}

void res_topo_init(void)
{
  host_t               *local_host;
  hwloc_topology_t      hwloc_topo;
  hwloc_obj_t           hwloc_root;
  resource_t           *local_root;

  hwloc_topology_init(&hwloc_topo);
  hwloc_topology_load(hwloc_topo);
  hwloc_root = hwloc_get_root_obj(hwloc_topo);
  local_root = traverse_topo(hwloc_topo, hwloc_root, DEPTH_ZERO, NULL);
  hwloc_topology_destroy(hwloc_topo);

  local_host = res_host_create(res_hostname(), LOCAL_HOST, local_root);

  res_topo_create();
  res_topo_add_host(local_host, local_host->index);
}

void res_topo_destroy(void)
{
  int i;
  for (i = 0; i < global_topo->size; ++i) {
    res_host_destroy(global_topo->host[i]);
  }
  xfree(global_topo->host);
  xfree(global_topo);
  global_topo = NULL;
}

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
}

