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

/* A host is a set of tightly connected
 * hardware resources which are managed
 * by an operating system. */
struct host {
  char         *name;
  int           index;
  resource_t   *root;
  int           size;
  resource_t   *cores;
  int           ncores;
  resource_t   *procs;
  int           nprocs;
};

/* There is only one global topology.
 * It consists of a set of hosts. */
struct topo {
  host_t      **host;
  int           size;
};

#endif
