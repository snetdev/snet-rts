#ifndef _XWORKER_H
#define _XWORKER_H

/* A worker can be in either of two roles: */
typedef enum worker_role {
  /* Data workers process records. */
  DataWorker,

  /* Input manager processes incoming connections. */
  InputManager,
} worker_role_t;

/* A work item represents a license to read from a descriptor. */
typedef struct work_item {
  /* A pointer to the next work item in the to-do list. */
  struct work_item      *next_item;

  /* A pointer to the next work item in the freed work item list. */
  struct work_item      *next_free;

  /* A pointer to the stream which contains at least 'count' records. */
  snet_stream_desc_t    *desc;

  /* The number of records in the 'desc' stream which we are allowed to read. */
  int                    count;

  /* An exclusive lock to arbitrate between the worker and visiting thieves. */
  int                    lock;

  /* A counter which refers to the value of the worker's turn value
   * at the moment of appending it to the list of free work items.*/
  size_t                 turn;
} work_item_t;

/* A list of to-do work items. */
typedef struct work_list {
  /* The head is an unused item: this simplifies modification operations. */
  work_item_t            head;
} work_list_t;

/* A mutual exclusion lock for visiting thieves: only one thief at a time. */
typedef struct worker_lock {
  /* The worker ID of a visiting thief iff non-zero. */
  int                    id;
} worker_lock_t;

/* A monotonically increasing counter for every thief visit and departure. */
typedef struct worker_turn {
  size_t                 turn;
} worker_turn_t;

/* A list of freed work items which can be reused if "turn" allows it. */
typedef struct worker_free_list {
  /* The first element of the list: where to retrieve items from. */
  work_item_t           *head;

  /* The last element of the list: where to append items to. */
  work_item_t           *tail;

  /* The number of elements in this list. */
  int                    count;
} worker_free_list_t;

/* The booty of a successful thief. */
typedef struct worker_loot {
  /* The pointer to the stream, which is non-NULL iff a steal is in progress. */
  snet_stream_desc_t    *desc;

  /* A pointer to the thief's own to-do work list iff it has a 'desc' work item. */
  work_item_t           *item;

  /* The number of stolen read licenses for 'desc'. */
  int                    count;
} worker_loot_t;

/* A worker with a pile of work todo.
 * Work is taken either from the todo list
 * or else from the input_node.
 */
struct worker {
  /* A unique ID used in locking to identify owners. */
  int                    id;

  /* The role a worker can be in: data worker or manager. */
  worker_role_t          role;

  /* The ID of the current or last visited worker when stealing. */
  int                    victim_id;

  /* A list of work to be done. */
  work_list_t            todo;

  /* An iterator over the to-do list. */
  work_item_t           *iter;

  /* The previous position of the iterator. */
  work_item_t           *prev;

  /* A list cache of freed work items. */
  worker_free_list_t     free;

  /* The booty of a successful thief. */
  worker_loot_t          loot;

  /* A pointer to the input stream. */
  snet_stream_desc_t    *input_desc;

  /* The output node which stores the number of output records. */
  struct node           *output_node;

  /* An exclusive lock for visiting thieves. */
  worker_lock_t         *steal_lock;

  /* A counter which is incremented when a thief enters or leaves. */
  worker_turn_t         *steal_turn;

  /* A hash table which maps stream descriptors to work items. */
  struct hash_ptab      *hash_ptab;

  /* A continuation store used by the streams layer for continuous processing. */
  snet_stream_desc_t    *continue_desc;
  snet_record_t         *continue_rec;

  /* Whether the worker has any work to be done at all. */
  bool                   has_work;

  /* Whether the input node has not yet reached end-of-file. */
  bool                   has_input;

  /* Whether this worker is busy processing. */
  int                    is_idle;

  /* The sequence number for the current idle state. */
  size_t                 idle_seqnr;
};


#endif
