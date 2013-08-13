#ifndef _NODE_H_
#define _NODE_H_

#if !defined(BUFSIZ) || !defined(_IOLBF) 
#include <stdio.h>
#endif
#if !defined(_ERRNO_H)
#include <errno.h>
#endif
#if !defined(_SCHED_H)
#include <sched.h>
#endif
#if !defined(_ASSERT_H)
#include <assert.h>
#endif

#include "stream.h"
#include "interface.h"
#include "label.h"
#include "variant.h"
#include "record.h"
#include "snettypes.h"
#include "type.h"
#include "memfun.h"
#include "hashtab.h"
#include "debug.h"
#include "trace.h"
#include "xstack.h"
#include "xfifo.h"
#include "xlock.h"

/* The root node in distributed S-Net. */
#define ROOT_LOCATION           0

/* Location may be determined by indexed placement combinators. */
#define LOCATION_UNKNOWN        (-1)

typedef struct node node_t;
typedef enum node_type node_type_t;
typedef enum landing_type landing_type_t;
typedef struct snet_stream_desc_t snet_stream_desc_t;
#ifndef _RECORD_H_
typedef struct snet_entity_t snet_entity_t;
#endif
typedef struct worker worker_t;
typedef struct landing landing_t;
typedef struct hash_ptab hash_ptab_t;

#include "xworker.h"
#include "detref.h"

typedef void (*node_work_fun_t)(snet_stream_desc_t *desc, snet_record_t *rec);
typedef void (*node_stop_fun_t)(node_t *node, fifo_t *fifo);
typedef void (*node_term_fun_t)(landing_t *landing, fifo_t *fifo);

/* Enumerate network node subtypes */
enum node_type {
  NODE_box       = 200,
  NODE_parallel  = 210,
  NODE_star      = 220,
  NODE_split     = 230,
  NODE_feedback  = 240,
  NODE_sync      = 250,
  NODE_filter    = 260,
  NODE_nameshift = 270,
  NODE_collector = 280,
  NODE_input     = 290,
  NODE_output    = 300,
  NODE_detenter  = 310,
  NODE_detleave  = 320,
  NODE_zipper    = 330,
  NODE_identity  = 340,
  NODE_garbage   = 350,
  NODE_observer  = 360,
  NODE_observer2 = 370,
  NODE_dripback  = 380,
  NODE_transfer  = 390,
  NODE_imanager  = 400,
};

enum landing_type {
  LAND_siso      = 500,
  LAND_collector = 510,
  LAND_feedback1 = 520,
  LAND_feedback2 = 530,
  LAND_feedback3 = 540,
  LAND_feedback4 = 550,
  LAND_parallel  = 560,
  LAND_split     = 570,
  LAND_star      = 580,
  LAND_sync      = 590,
  LAND_zipper    = 620,
  LAND_identity  = 630,
  LAND_input     = 640,
  LAND_garbage   = 650,
  LAND_observer  = 660,
  LAND_empty     = 670,
  LAND_box       = 680,
  LAND_dripback1 = 690,
  LAND_dripback2 = 700,
  LAND_transfer  = 710,
  LAND_imanager  = 720,
  LAND_remote    = 730,
};

/* A stream is a static edge in the node graph. */
struct snet_stream_t {
  node_t *from;         /* originating node */
  node_t *dest;         /* destination node */
  int     table_index;  /* index in stream table */
};
#define STREAM_FROM(stream)   ((stream)->from)
#define STREAM_DEST(stream)   ((stream)->dest)

/* Argument for box nodes */
typedef struct box_arg {
  snet_stream_t        **outputs;
  snet_handle_t*        (*boxfun)( snet_handle_t*);
  snet_int_list_list_t  *output_variants;
  const char            *boxname;
  snet_variant_list_t   *vars;
  snet_entity_t         *entity;
  int                    concurrency;
  bool                   is_det;
} box_arg_t;

/* Argument for collector nodes */
typedef struct collector_arg {
  snet_stream_t        *output;
  int                   num;
  bool                  is_det;
  bool                  is_detsup;
  node_t               *peer;
  int                   stopped;
  snet_entity_t        *entity;
} collector_arg_t;

/* Argument for dripback nodes */
typedef struct dripback_arg {
  snet_stream_t        *input;
  snet_stream_t        *output;
  snet_stream_t        *instance;
  snet_stream_t        *dripback;
  snet_stream_t        *selfref;
  snet_variant_list_t  *back_patterns;
  snet_expr_list_t     *guards;
  int                   stopping;
  snet_entity_t        *entity;
} dripback_arg_t;

/* Argument for feedback nodes */
typedef struct feedback_arg {
  snet_stream_t        *input;
  snet_stream_t        *output;
  snet_stream_t        *instance;
  snet_stream_t        *feedback;
  snet_stream_t        *selfref2;
  snet_stream_t        *selfref4;
  snet_variant_list_t  *back_patterns;
  snet_expr_list_t     *guards;
  int                   stopping;
  snet_entity_t        *entity;
} feedback_arg_t;

/* Argument for filter nodes */
typedef struct filter_arg {
  snet_stream_t         *output;
  snet_variant_t        *input_variant;
  snet_expr_list_t      *guard_exprs;
  snet_filter_instr_list_list_t **filter_instructions;
  snet_entity_t         *entity;
} filter_arg_t;

/* The state an input node can be in. */
typedef enum input_state {
  INPUT_reading,
  INPUT_terminating,
  INPUT_terminated,
} input_state_t;

/* Argument for input nodes */
typedef struct input_arg {
  snet_stream_t         *output;
  snet_stream_desc_t    *indesc;
  input_state_t          state;
} input_arg_t;

/* Argument for output nodes */
typedef struct output_arg {
  FILE                  *file;
  snetin_label_t        *labels;
  snetin_interface_t    *interfaces;
  size_t                 num_outputs;
  bool                   terminated;
} output_arg_t;

/* Argument for parallel dispatcher nodes */
typedef struct parallel_arg {
  snet_stream_t         **outputs;
  snet_variant_list_list_t *variant_lists;
  int                   num;
  bool                  is_det;
  bool                  is_detsup;
  snet_stream_t         *collector;
  snet_entity_t         *entity;
} parallel_arg_t;

/* Argument for split nodes */
typedef struct split_arg {
  snet_stream_t         *collector;
  snet_stream_t         *instance;
  int                   ltag;
  int                   utag;
  bool                  is_det;
  bool                  is_detsup;
  bool                  is_byloc;
  snet_entity_t         *entity;
} split_arg_t;

/* Argument for the star nodes */
typedef struct star_arg {
  snet_stream_t        *input;
  snet_stream_t        *collector;
  snet_stream_t        *instance;
  snet_stream_t        *internal;
  snet_variant_list_t  *exit_patterns;
  snet_expr_list_t     *guards;
  bool                  is_det;
  bool                  is_detsup;
  bool                  is_incarnate;
  int                   stopping;
  snet_entity_t        *entity;
} star_arg_t;

/* Argument for synchrocell nodes */
typedef struct sync_arg {
  snet_stream_t        *output;
  snet_variant_list_t  *patterns;
  snet_expr_list_t     *guard_exprs;
  int                   num_patterns;
  snet_variant_t       *merged_pattern;
  snet_entity_t        *entity;
  bool                  garbage_collect;
  bool                  merged_type_sync;
} sync_arg_t;

/* Argument for zipper nodes */
typedef struct zipper_arg {
  snet_stream_t        *output;
  snet_variant_list_t  *exit_patterns;
  snet_expr_list_t     *exit_guards;
  snet_variant_list_t  *sync_patterns;
  snet_expr_list_t     *sync_guards;
  unsigned int          sync_width;
  snet_entity_t        *entity;
} zipper_arg_t;

/* Argument for the file observer 1. */
typedef struct observer_arg {
  snet_stream_t        *output1;
  snet_stream_t        *output2;
  const char           *filename;
  const char           *address;
  int                   port;
  bool                  interactive;
  const char           *position;
  const char           *code;
  int                   type;
  int                   level;
} observer_arg_t;

/* Argument for input manager */
typedef struct imanager_arg {
  snet_stream_t        *input;
  snet_stream_desc_t   *indesc;
} imanager_arg_t;

/* Network node */
struct node {
  node_type_t           type;
  node_work_fun_t       work;
  node_stop_fun_t       stop;
  node_term_fun_t       term;
  int                   location;
  int                   loc_split_level;
  int                   subnet_level;
  union node_types {
    box_arg_t           box;
    collector_arg_t     collector;
    dripback_arg_t      dripback;
    feedback_arg_t      feedback;
    filter_arg_t        filter;
    input_arg_t         input;
    output_arg_t        output;
    parallel_arg_t      parallel;
    split_arg_t         split;
    star_arg_t          star;
    sync_arg_t          sync;
    zipper_arg_t        zipper;
    observer_arg_t      observer;
    imanager_arg_t      imanager;
  } u;
};
#define NODE_TYPE(nptr)        ((nptr)->type)
#define NODE_SPEC(nptr, arg)   (&((nptr)->u.arg))

/* Node instantiation for the most common SISO node types. */
typedef struct landing_siso {
  snet_stream_desc_t   *outdesc;
} landing_siso_t;

/* Entry point for deterministic networks */
typedef struct landing_detenter {
  long                  counter;
  long                  seqnr;
  fifo_t               *detfifo;
  landing_t            *collland;
} landing_detenter_t;

/* The state of a worker when it processes a record in a concurrent box. */
typedef struct box_context {
  snet_stream_desc_t   *outdesc;
  snet_record_t        *rec;
  landing_t            *land;
  int                   index;
  bool                  busy;
} box_context_t;

/* Extend a box context to cache line size. */
typedef struct box_context_union {
  box_context_t         context;
  unsigned char         unused[LINE_SIZE];
} box_context_union_t;

/* Node instantiation for a box. */
typedef struct landing_box {
  int                   concurrency;
  box_context_union_t  *contexts;
  landing_t            *collland;
  landing_detenter_t    detenter;
} landing_box_t;

/* Node instantiation for collectors */
typedef struct landing_collector {
  snet_stream_desc_t   *outdesc;
  fifo_t                detfifo;
  long                  counter;
  landing_t            *peer;
} landing_collector_t;

/* First dripback landing: access */
typedef struct landing_dripback1 {
  landing_t            *dripback2;
  snet_stream_desc_t   *controldesc;
  fifo_t               *recfifo;
} landing_dripback1_t;

/* Second dripback landing: control */
typedef struct landing_dripback2 {
  snet_stream_desc_t   *outdesc;
  snet_stream_desc_t   *instdesc;
  fifo_t                detfifo;
  fifo_t                recfifo;
  long                  queued;
  long                  entered;
  enum DripBackState {
      DripBackIdle,
      DripBackBusy,
  } state;
  enum DripBackTerminateState {
    DripBackInitial,
    DripBackDraining,
    DripBackTerminated,
  } terminate;
} landing_dripback2_t;

/* First feedback landing: access */
typedef struct landing_feedback1 {
  landing_t            *feedback2;
  landing_t            *feedback3;
  landing_t            *feedback4;
  snet_stream_desc_t   *instdesc;
  snet_stream_desc_t   *outdesc;
  fifo_t               *detfifo;
  long                  counter;
} landing_feedback1_t;

/* Third feedback landing: return */
typedef struct landing_feedback3 {
  landing_t            *feedback2;
  landing_t            *feedback4;
  snet_stream_desc_t   *instdesc;
  snet_stream_desc_t   *outdesc;
  fifo_t                detfifo;
  enum FeedbackTerminateState {
    FeedbackInitial,
    FeedbackDraining,
    FeedbackTerminating,
    FeedbackTerminated,
  } terminate;
} landing_feedback3_t;

/* Second feedback landing: instance */
typedef struct landing_feedback2 {
  snet_stream_desc_t   *instdesc;
  landing_t            *feedback3;
} landing_feedback2_t;

/* Fourth feedback landing: exit */
typedef struct landing_feedback4 {
  snet_stream_desc_t   *outdesc;
} landing_feedback4_t;

/* Keep track of whether a record matches a variant list. */
typedef struct match_count {
  bool is_match;
  int count;
} match_count_t;

/* Node instantiation for parallel */
typedef struct landing_parallel {
  snet_stream_desc_t  **outdescs;
  snet_stream_desc_t   *colldesc;
  match_count_t        *matchcounter;
  landing_t            *collland;
  bool                 *terminated;
  bool                  initialized;
  landing_detenter_t    detenter;
} landing_parallel_t;

/* Node instantiation for split */
typedef struct landing_split {
  snet_stream_desc_t   *colldesc;
  hashtab_t            *hashtab;
  int                   split_count;
  landing_t            *collland;
  landing_detenter_t    detenter;
} landing_split_t;

/* Node instantiation for star */
typedef struct landing_star {
  snet_stream_desc_t   *colldesc;
  snet_stream_desc_t   *instdesc;
  landing_t            *collland;
  landing_t            *instland;
  int                   incarnation_count;
  bool                  star_leader;
  landing_detenter_t    detenter;
} landing_star_t;

/* The state a sync node can be in. */
typedef enum {
  SYNC_initial,  /* no records matched. */
  SYNC_partial,  /* one or more records matched. */
  SYNC_complete, /* all patterns matched -> now transparent. */
} sync_state_t;

/* Node instantiation for sync */
typedef struct landing_sync {
  snet_stream_desc_t   *outdesc;
  snet_record_t       **storage;
  int                   match_count;
  sync_state_t          state;
} landing_sync_t;

/* Node instantiation for transfer, i.e. outgoing connections to other locations. */
typedef struct landing_transfer {
  int                   dest_loc;
  int                   table_index;
  int                   split_level;
  struct connect       *connect;
  snet_stream_t        *stream;
} landing_transfer_t;

/* Node instantiation for remote, i.e. continuations of incoming connections. */
typedef struct landing_remote {
  int                   cont_loc;
  int                   cont_num;
  int                   split_level;
  int                   dyn_loc;
  void                (*stackfunc)(landing_t *, snet_stream_desc_t *);
  void                (*retfunc)(landing_t *, snet_stream_desc_t *, snet_stream_desc_t *);
} landing_remote_t;

/* Node instantiation for zipper */
typedef struct matching matching_t;
typedef struct landing_zipper {
  snet_stream_desc_t   *outdesc;
  matching_t           *head;
} landing_zipper_t;

/* Node instantiation for identity */
typedef struct landing_identity {
  snet_stream_desc_t   *outdesc;        /* the usual outgoing descriptor */
  landing_type_t        prev_type;      /* type before becoming an identity */
} landing_identity_t;

/* Node instantiation for input */
typedef struct landing_input {
  snet_stream_desc_t   *outdesc;        /* the usual outgoing descriptor */
  size_t                num_inputs;     /* number of records input */
} landing_input_t;

/* Node instantiation for an observer */
typedef struct landing_observer {
  snet_stream_desc_t   *outdesc;
  snet_stream_desc_t   *outdesc2;
  int                   oid;
} landing_observer_t;

/* Node instantiation for an observer2 */
typedef struct landing_empty {
} landing_empty_t;

/* A landing instantiates a network node.
 * It connects incoming with outgoing stream descriptors.
 * It is locked by a worker to enforce sequential order. */
struct landing {
  landing_type_t        type;           /* determine landing type in union */
  node_t               *node;           /* node in the fixed network */
  snet_stack_t          stack;          /* a stack of future landings */
  int                  *dyn_locs;       /* indexed placement locations */
  worker_t             *worker;         /* which worker has locked the landing */
  int                   id;             /* lock */
  int                   refs;           /* reference counter */
  union landing_types {
    landing_siso_t      siso;
    landing_box_t       box;
    landing_collector_t collector;
    landing_dripback1_t dripback1;
    landing_dripback2_t dripback2;
    landing_feedback1_t feedback1;
    landing_feedback2_t feedback2;
    landing_feedback3_t feedback3;
    landing_feedback4_t feedback4;
    landing_parallel_t  parallel;
    landing_split_t     split;
    landing_star_t      star;
    landing_sync_t      sync;
    landing_zipper_t    zipper;
    landing_identity_t  identity;
    landing_input_t     input;
    landing_observer_t  observer;
    landing_empty_t     empty;
    landing_transfer_t  transfer;
    landing_remote_t    remote;
  } u;
};
#define LAND_SPEC(land,type)            (&(land)->u.type)
#define LAND_NODE_SPEC(land,type)       NODE_SPEC((land)->node, type)
#define LAND_INCR(land)                 AAF(&(land)->refs, 1)
#define LAND_DECR(land)                 SAF(&(land)->refs, 1)

/* A stream descriptor is the instantiation of a stream. */
struct snet_stream_desc_t {
  snet_stream_t        *stream;         /* stream between two nodes */
  landing_t            *landing;        /* destination landing */
  landing_t            *source;         /* originating landing */
  fifo_t                fifo;           /* a FIFO queue for records */
  int                   refs;           /* reference counter */
};
#define DESC_LAND_SPEC(desc,type)       LAND_SPEC((desc)->landing, type)
#define DESC_NODE(desc)                 ((desc)->landing->node)
#define DESC_NODE_SPEC(desc,type)       NODE_SPEC(DESC_NODE(desc), type)
#define DESC_STREAM(desc)               ((desc)->stream)
#define DESC_DEST(desc)                 STREAM_DEST(DESC_STREAM(desc))
#define DESC_FROM(desc)                 STREAM_FROM(DESC_STREAM(desc))
#define DESC_INCR(desc)                 AAF(&(desc)->refs, 1)
#define DESC_DECR(desc)                 SAF(&(desc)->refs, 1)
#define DESC_DECR_MULTI(desc,amount)    SAF(&(desc)->refs, amount)
#define DESC_LOCK(desc)                 ((desc)->landing->id)

#define CAS(ptr, old, val)      __sync_bool_compare_and_swap(ptr, old, val)
#define AAF(ptr, val)           __sync_add_and_fetch(ptr, val)
#define FAA(ptr, val)           __sync_fetch_and_add(ptr, val)
#define SAF(ptr, val)           __sync_sub_and_fetch(ptr, val)
#define FAS(ptr, val)           __sync_fetch_and_sub(ptr, val)
#define BAR()                   __sync_synchronize()

#define CAS_LOCKING     1

static inline void lock_landing(landing_t *landing, worker_t *worker)
{
#if CAS_LOCKING
  /* destination landing must be unlocked */
  assert(landing->id == 0);
  assert(landing->worker == NULL);
  /* lock destination landing */
  while (!CAS(&landing->id, 0, worker->id)) {
    sched_yield();
  }
  assert(landing->id == worker->id);
  landing->worker = worker;
#elif SEM_LOCKING
  if (LOCK(landing->lock)) {
    assert(0);
  }
  landing->worker = worker;
  landing->id = worker->id;
#else
  #error unspecified locking
#endif
}

static inline bool trylock_landing(landing_t *landing, worker_t *worker)
{
#if CAS_LOCKING
  if (CAS(&landing->id, 0, worker->id)) {
    landing->worker = worker;
    return true;
  }
#elif SEM_LOCKING
  if (TRYLOCK(landing->lock)) {
    if (errno != EAGAIN) {
      assert(0);
    }
  } else {
    landing->worker = worker;
    landing->id = worker->id;
    return true;
  }
#else
  #error unspecified locking
#endif
  return false;
}

static inline void unlock_landing(landing_t *landing)
{
#if CAS_LOCKING
  /* destination landing must be locked */
  assert(landing->id);
  /* unlock destination landing */
  landing->worker = NULL;
  if (!CAS(&landing->id, landing->id, 0)) {
    assert(0);
  }
#elif SEM_LOCKING
  assert(landing->id > 0);
  landing->id = 0;
  landing->worker = NULL;
  UNLOCK(landing->lock);
#else
  #error unspecified locking
#endif
}

static inline bool trylock_worker(worker_t *victim, worker_t *thief)
{
#if CAS_LOCKING
  if (CAS(&victim->steal_lock->id, 0, thief->id)) {
    return true;
  }
#else
  #error unspecified locking
#endif
  return false;
}

static inline void unlock_worker(worker_t *victim, worker_t *thief)
{
#if CAS_LOCKING
  /* victim must be locked */
  assert(victim->steal_lock->id == thief->id);
  /* unlock victim */
  if (!CAS(&victim->steal_lock->id, thief->id, 0)) {
    assert(0);
  }
#else
  #error unspecified locking
#endif
}

#define lock_work_item(node, worker)            \
  if (CAS(&(node)->lock, 0, (worker)->id))      \
    ; else assert(0)

#define trylock_work_item(node, worker)         \
  CAS(&(node)->lock, 0, (worker)->id)

#define unlock_work_item(node, worker)          \
  /* node must be locked */                     \
  if (assert((node)->lock == (worker)->id),     \
  /* unlock work item */                        \
     CAS(&(node)->lock, (worker)->id, 0)) {     \
  } else assert(0)

#include "node-proto.h"

#endif
