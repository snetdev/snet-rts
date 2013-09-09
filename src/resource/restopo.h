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
  core_t        *core;          /* parent core */
  proc_state_t   state;         /* allocated? */
  int            clientbit;     /* owner */
  int            logical;       /* hwloc logical number */
  int            physical;      /* identifier for OS */
};

/* A core with one or more hyperthreaded procs. */
struct core {
  int            hyper;         /* degree of hyperthreading */
  int            nprocs;        /* number of child processors */
  proc_t       **procs;         /* child processors */
  cache_t       *cache;         /* parent node */
  int            assigned;      /* count of assigned procs */
  int            logical;       /* hwloc logical number */
  int            physical;      /* identifier for OS */
};

/* A level 3 cache. */
struct cache {
  int            ncores;        /* number of child cores */
  core_t       **cores;         /* child cores */
  int            nprocs;        /* total number of child procs */
  numa_t        *numa;          /* parent node */
  int            assigned;      /* count of assigned procs */
  int            logical;       /* hwloc logical number */
  int            physical;      /* identifier for OS */
};

/* A NUMA node. */
struct numa {
  int            ncaches;       /* number of child caches */
  cache_t      **caches;        /* child caches */
  int            ncores;        /* total number of child cores */
  int            nprocs;        /* total number of child procs */
  host_t        *host;          /* parent node */
  int            assigned;      /* count of assigned procs */
  int            logical;       /* hwloc logical number */
  int            physical;      /* identifier for OS */
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
