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

struct res {
  int           depth;
  res_kind_t    kind;
  int           logical;
  int           physical;
  int           cache_level;
  struct res   *parent;
  struct res  **children;
  int           num_children;
  int           max_children;
  int           first_core;
  int           last_core;
  int           first_proc;
  int           last_proc;
};

#endif
