#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include "resdefs.h"

typedef struct proc proc_t;
typedef struct core core_t;
typedef struct cache cache_t;
typedef struct numa numa_t;
typedef struct host host_t;
typedef struct topo topo_t;

#include "resource.h"
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
  resource_t   *local_root = res_resource_init();
  host_t       *local_host;

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

