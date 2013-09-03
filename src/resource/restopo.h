#ifndef RESTOPO_H_INCLUDED
#define RESTOPO_H_INCLUDED

enum res_kind {
  System,
  Machine,
  Node,
  Socket,
  Cache,
  Core,
  Proc,
  Other,
};

struct resource {
  int           depth;
  res_kind_t    kind;
  int           logical;
  int           physical;
  int           cache_level;
  resource_t   *parent;
  resource_t  **children;
  int           num_children;
  int           max_children;
  int           first_core;
  int           last_core;
  int           first_proc;
  int           last_proc;
};

/* A schedulable processor unit. */
struct proc {
  resource_t    *res;
  core_t        *core;
  int            assigned;
  client_t      *client;
};

/* A core with one or more hyperthreaded procs. */
struct core {
  resource_t    *res;
  int            hyper;
  int            nprocs;
  proc_t       **procs;
  cache_t       *cache;
  int            assigned;
};

/* A level 3 cache. */
struct cache {
  resource_t    *res;
  int            ncores;
  core_t       **cores;
  int            nprocs;
  numa_t        *numa;
  int            assigned;
};

/* A NUMA node. */
struct numa {
  resource_t    *res;
  int            ncaches;
  cache_t      **caches;
  int            ncores;
  int            nprocs;
  host_t        *host;
  int            assigned;
};

/* A host is a set of tightly connected
 * hardware resources which are managed
 * by an operating system. */
struct host {
  char          *name;
  resource_t    *root;
  int            index;
  int            size;
  numa_t       **numa;
  int            ncores;
  int            nprocs;
  core_t       **cores;
  proc_t       **procs;
  int            assigned;
};

/* There is only one global topology.
 * It consists of a set of hosts. */
struct topo {
  host_t       **host;
  int            size;
};

#endif
