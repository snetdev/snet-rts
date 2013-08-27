#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "resdefs.h"

typedef enum res_kind {
  System = 10,
  Machine,
  Node,
  Socket,
  Cache,
  Core,
  Proc,
  Other,
} res_kind_t;

typedef struct res {
  int           depth;
  res_kind_t    kind;
  struct res   *parent;
  struct res  **children;
  int           num_children;
  int           max_children;
  int           first_pu;
  int           last_pu;
} res_t;

static res_t *root;

static void res_relate(res_t *parent, res_t *child)
{
  if (parent->num_children == parent->max_children) {
    parent->max_children = parent->max_children
                         ? 2 * parent->max_children
                         : 1;
    parent->children = xrealloc(parent->children,
                                parent->max_children * sizeof(res_t *));
  }
  assert(parent->num_children < parent->max_children);
  parent->children[parent->num_children++] = child;
}

static res_t* res_add(res_t *parent, res_kind_t kind, int depth)
{
  res_t *res = xnew(res_t);
  res->depth = depth;
  res->kind = kind;
  res->parent = parent;
  res->children = 0;
  res->num_children = 0;
  res->max_children = 0;
  res->first_pu = -1;
  res->last_pu = -1;
  if (parent == NULL) {
    assert(root == NULL);
    root = res;
  } else {
    res_relate(parent, res);
  }
  return res;
}

static res_kind_t res_type_to_kind(hwloc_obj_type_t type)
{
  res_kind_t kind = Other;
  switch (type) {
    case HWLOC_OBJ_SYSTEM: kind = System; break;
    case HWLOC_OBJ_MACHINE: kind = Machine; break;
    case HWLOC_OBJ_NODE: kind = Node; break;
    case HWLOC_OBJ_SOCKET: kind = Socket; break;
    case HWLOC_OBJ_CACHE: kind = Cache; break;
    case HWLOC_OBJ_CORE: kind = Core; break;
    case HWLOC_OBJ_PU: kind = Proc; break;
    default:
      res_error("Invalid type %s\n", hwloc_obj_type_string(type));
  }
  return kind;
}

static void traverse_topo(
  hwloc_topology_t topo,
  hwloc_obj_t obj,
  int depth,
  res_t* parent)
{
  int i;
  res_kind_t kind;
  res_t* res;

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
      return;
  }
  res = res_add(parent, kind, depth);
  if (res->kind == Proc) {
    assert(obj->arity == 0);
    res->first_pu = res->last_pu = obj->logical_index;
  }
  else {
    for (i = 0; i < obj->arity; i++) {
      traverse_topo(topo, obj->children[i], depth + 1, res);
    }
    for (i = 0; i < res->num_children; i++) {
      if (i == 0) {
        res->first_pu = res->children[i]->first_pu;
        res->last_pu = res->children[i]->last_pu;
      } else {
        if (res->children[i]->first_pu != res->last_pu + 1) {
          res_warn("Gap at depth %d, child %d, %d -> %d\n",
                   depth, i, res->last_pu, res->children[i]->first_pu);
        }
        res->last_pu = res->children[i]->last_pu;
      }
    }
  }
  res_info("Depth %d range %d -> %d\n", depth, res->first_pu, res->last_pu);
}

static void print_children(hwloc_topology_t topology, hwloc_obj_t obj,
                           int depth)
{
  char string[128];
  unsigned i;

  hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
  printf("%*s%s\n", 2 * depth, "", string);
  for (i = 0; i < obj->arity; i++) {
    print_children(topology, obj->children[i], depth + 1);
  }
}

int hwinit(void)
{
  int depth;
  unsigned i, n;
  unsigned long size;
  int levels;
  char string[128];
  int topodepth;
  hwloc_topology_t topology;
  hwloc_cpuset_t cpuset;
  hwloc_obj_t obj;

  /* Allocate and initialize topology object. */
  hwloc_topology_init(&topology);

  /* Perform the topology detection. */
  hwloc_topology_load(topology);

  traverse_topo(topology, hwloc_get_root_obj(topology), 0, root);
  if (root) {
    hwloc_topology_destroy(topology);
    return 0;
  }

  /* Optionally, get some additional topology information
     in case we need the topology depth later. */
  topodepth = hwloc_topology_get_depth(topology);

 /*****************************************************************
  * First example:
  * Walk the topology with an array style, from level 0 (always
  * the system level) to the lowest level (always the proc level).
  *****************************************************************/
  for (depth = 0; depth < topodepth; depth++) {
    printf("*** Objects at level %d\n", depth);
    for (i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
      hwloc_obj_snprintf(string, sizeof(string), topology,
                         hwloc_get_obj_by_depth(topology, depth, i), "#", 0);
      printf("Index %u: %s\n", i, string);
    }
  }

 /*****************************************************************
  * Second example:
  * Walk the topology with a tree style.
  *****************************************************************/
  printf("*** Printing overall tree\n");
  print_children(topology, hwloc_get_root_obj(topology), 0);

 /*****************************************************************
  * Third example:
  * Print the number of sockets.
  *****************************************************************/
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  if (depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
    printf("*** The number of sockets is unknown\n");
  }
  else {
    printf("*** %u socket(s)\n", hwloc_get_nbobjs_by_depth(topology, depth));
  }

 /*****************************************************************
  * Fourth example:
  * Compute the amount of cache that the first logical processor
  * has above it.
  *****************************************************************/
  levels = 0;
  size = 0;
  for (obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
       obj; obj = obj->parent)
    if (obj->type == HWLOC_OBJ_CACHE) {
      levels++;
      size += obj->attr->cache.size;
    }
  printf("*** Logical processor 0 has %d caches totaling %luKB\n",
         levels, size / 1024);

 /*****************************************************************
  * Fifth example:
  * Bind to only one thread of the last core of the machine.
  *
  * First find out where cores are, or else smaller sets of CPUs if
  * the OS doesn't have the notion of a "core".
  *****************************************************************/
  depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);

  /* Get last core. */
  obj = hwloc_get_obj_by_depth(topology, depth,
                               hwloc_get_nbobjs_by_depth(topology, depth) - 1);
  if (obj) {
    /* Get a copy of its cpuset that we may modify. */
    cpuset = hwloc_bitmap_dup(obj->cpuset);

    /* Get only one logical processor (in case the core is
       SMT/hyperthreaded). */
    hwloc_bitmap_singlify(cpuset);

    /* And try to bind ourself there. */
    if (hwloc_set_cpubind(topology, cpuset, 0)) {
      char *str;
      int error = errno;
      hwloc_bitmap_asprintf(&str, obj->cpuset);
      printf("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
      free(str);
    }

    /* Free our cpuset copy */
    hwloc_bitmap_free(cpuset);
  }

 /*****************************************************************
  * Sixth example:
  * Allocate some memory on the last NUMA node, bind some existing
  * memory to the last NUMA node.
  *****************************************************************/
  /* Get last node. */
  n = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
  if (n) {
    void *m;
    size = 1024 * 1024;

    obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, n - 1);
    m = hwloc_alloc_membind_nodeset(topology, size, obj->nodeset,
                                    HWLOC_MEMBIND_DEFAULT, 0);
    hwloc_free(topology, m, size);

    m = malloc(size);
    hwloc_set_area_membind_nodeset(topology, m, size, obj->nodeset,
                                   HWLOC_MEMBIND_DEFAULT, 0);
    free(m);
  }

  /* Destroy topology object. */
  hwloc_topology_destroy(topology);

  return 0;
}
