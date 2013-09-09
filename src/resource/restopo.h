#ifndef RESTOPO_H_INCLUDED
#define RESTOPO_H_INCLUDED

/* A processor cycles through the following states. */
enum proc_state {
  Avail,        // When a processor is available for scheduling.
  Grant,        // When the resource server has allocated it to a client.
  Accept,       // When the client has acknowledged the allocation.
  Revoke,       // When the server tells the client to stop using it.
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
  char          *hostname;
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

#endif
