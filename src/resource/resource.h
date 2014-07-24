#ifndef RESOURCE_H_INCLUDED
#define RESOURCE_H_INCLUDED

/* The kind of hardware resource. Similar to HWLOC. */
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

/* Commmon structure for a hardware resource. */
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

#endif
