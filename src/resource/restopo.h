#ifndef RESTOPO_H_INCLUDED
#define RESTOPO_H_INCLUDED

enum proc_state {
  Avail,
  Grant,
  Accept,
  Revoke,
};

/* A schedulable processor unit. */
struct proc {
  resource_t    *res;
  core_t        *core;
  proc_state_t   state;
  int            clientbit;
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
  int            nnumas;
  numa_t       **numa;
  int            ncores;
  int            nprocs;
  core_t       **cores;
  proc_t       **procs;
  bitmap_t       procassign;
  bitmap_t       coreassign;
};

/* There is only one global topology.
 * It consists of a set of hosts. */
struct topo {
  host_t       **host;
  int            nhosts;
};

#endif
