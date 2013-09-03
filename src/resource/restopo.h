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
  int            assigned;
};

/* A core with one or more hyperthreaded procs. */
struct core {
  resource_t    *res;
  int            hyper;
  int            nprocs;
  proc_t       **procs;
};

/* A level 3 cache. */
struct cache {
  resource_t    *res;
  int            ncores;
  core_t       **cores;
  int            nprocs;
};

/* A NUMA node. */
struct numa {
  resource_t    *res;
  int            ncaches;
  cache_t      **caches;
  int            ncores;
  int            nprocs;
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
};

/* There is only one global topology.
 * It consists of a set of hosts. */
struct topo {
  host_t       **host;
  int            size;
};

#endif
