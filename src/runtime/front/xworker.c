#include <unistd.h>
#include "node.h"

static worker_t       **snet_workers;
static int              snet_worker_count;
static int              snet_thief_limit;
static lock_t           snet_idle_lock;

/* Init worker data */
void SNetWorkerInit(void)
{
  snet_workers = SNetNodeGetWorkers();
  snet_worker_count = SNetNodeGetWorkerCount();
  snet_thief_limit = SNetThreadingThieves();
  if (snet_thief_limit > 1) {
    LOCK_INIT2(snet_idle_lock, snet_thief_limit);
  } else {
    LOCK_INIT(snet_idle_lock);
  }
}

/* Cleanup worker data */
void SNetWorkerCleanup(void)
{
  snet_worker_count = 0;
  snet_workers = NULL;
  LOCK_DESTROY(snet_idle_lock);
}

/* Initialize a new empty work list. */
static void WorkerTodoInit(work_list_t *q)
{
  q->head.next_item = NULL;
  q->head.next_free = NULL;
  q->head.desc = NULL;
  q->head.count = 0;
  q->head.lock = 0;
  q->head.turn = 0;
}

/* Initialize a new free item list. */
static void WorkerFreeInit(worker_free_list_t *free)
{
  free->head = NULL;
  free->tail = NULL;
  free->count = 0;
}

/* Create a new worker. */
worker_t *SNetWorkerCreate(
    node_t *input_node,
    int worker_id,
    node_t *output_node,
    worker_role_t role)
{
  worker_t *worker;

  trace(__func__);

  worker = SNetNewAlign(worker_t);
  worker->id = worker_id;
  worker->role = role;
  worker->victim_id = worker_id;

  WorkerTodoInit(&worker->todo);

  worker->prev = worker->iter = &worker->todo.head;

  WorkerFreeInit(&worker->free);

  if (input_node) {
    worker->input_desc = NODE_SPEC(input_node, input)->indesc;
    worker->has_input = true;
  } else {
    worker->input_desc = NULL;
    worker->has_input = false;
  }
  worker->output_node = output_node;
  worker->steal_lock = SNetNewAlign(worker_lock_t);
  worker->steal_lock->id = 0;
  worker->steal_turn = SNetNewAlign(worker_turn_t);
  worker->steal_turn->turn = 1;
  
  worker->loot.desc = NULL;
  worker->loot.count = 0;
  worker->loot.item = NULL;
  worker->hash_ptab = SNetHashPtrTabCreate(10, true);
  worker->continue_desc = NULL;
  worker->continue_rec = NULL;
  worker->has_work = true;
  worker->is_idle = false;
  worker->idle_seqnr = 0;

  return worker;
}

/* Destroy a worker. */
void SNetWorkerDestroy(worker_t *worker)
{
  trace(__func__);

  /* if (SNetVerbose()) {
    if (worker->id == 1) {
      printf("Created %u records\n", SNetGetRecCounter());
    }
  } */

  /* Verify hash table is empty. */
  if ( ! SNetHashPtrTabEmpty(worker->hash_ptab)) {
    snet_stream_desc_t *desc = SNetHashPtrFirst(worker->hash_ptab);
    do {
      work_item_t *item = SNetHashPtrLookup(worker->hash_ptab, desc);
      printf("%s(%d,%d): desc %p, count %d, refs %d\n", __func__,
             worker->id, worker->role, desc, item->count, desc->refs);
    } while ((desc = SNetHashPtrNext(worker->hash_ptab, desc)) != NULL);
  }

  /* Free hash table. */
  SNetHashPtrTabDestroy(worker->hash_ptab);

  /* Free the list of free work items. */
  while (worker->free.head) {
    work_item_t *item = worker->free.head;
    worker->free.head = item->next_free;
    SNetDelete(item);
  }

  /* Free lock */
  SNetDelete(worker->steal_lock);
  SNetDelete(worker->steal_turn);

  /* Free worker. */
  SNetDelete(worker);
}

/* Minimize the number of cached work items on the free list. */
static void ReduceWorkItems(worker_t *worker)
{
  work_item_t *item;
  while ((item = worker->free.head) != NULL &&
         (item->turn < worker->steal_turn->turn ||
          (item->turn == worker->steal_turn->turn &&
           worker->steal_lock->id == 0)))
  {
    if ((worker->free.head = item->next_free) == NULL) {
      worker->free.tail = NULL;
    }
    --worker->free.count;
    SNetDelete(item);
  }
}

/* Obtain a free work item from either the free list or the heap. */
static work_item_t *GetFreeWorkItem(worker_t *worker)
{
  work_item_t *item = worker->free.head;
  if (item && (item->turn < worker->steal_turn->turn ||
              (item->turn == worker->steal_turn->turn &&
               worker->steal_lock->id == 0)))
  {
    if ((worker->free.head = item->next_free) == NULL) {
      worker->free.tail = NULL;
    }
    --worker->free.count;
  }
  else {
    item = SNetNew(work_item_t);
  }
  item->turn = 0;
  item->next_free = NULL;
  item->count = 0;
  return item;
}

/* Add a work item to the free list. */
static void PutFreeWorkItem(worker_t *worker, work_item_t *item)
{
  assert(item->count == 0);
  assert(item->lock);
  item->next_item = NULL;
  item->next_free = NULL;
  item->desc = NULL;
  item->turn = worker->steal_turn->turn;
  if (worker->free.tail == NULL) {
    worker->free.tail = worker->free.head = item;
  } else {
    worker->free.tail->next_free = item;
    worker->free.tail = item;
  }
  ++worker->free.count;
}

/* Add a new unit of work to the worker's todo list.
 * At return iterator should point to the new item.
 */
void SNetWorkerTodo(worker_t *worker, snet_stream_desc_t *desc)
{
  work_item_t   *item = SNetHashPtrLookup(worker->hash_ptab, desc);

  if (item) {
    /* Item may be locked by a thief. */
    AAF(&item->count, 1);
  } else {
    item = GetFreeWorkItem(worker);
    item->count = 1;
    item->desc = desc;
    item->lock = 0;
    item->next_free = NULL;
    item->next_item = worker->prev->next_item;
    BAR();
    worker->prev->next_item = item;
    worker->prev = item;
    SNetHashPtrStore(worker->hash_ptab, desc, item);
    worker->has_work = true;
  }
}


/* Work on an item.
 *
 * In case of a dissolved garbage stream update the source
 * stream of the work item.
 *
 * Return true iff a record was processed.
 *
 * If the contents of the item was merged with another item then
 * reset the item descriptor to NULL.
 */
static bool SNetWorkerWorkItem(work_item_t *const item, worker_t *worker)
{
  work_item_t           *lookup;

  /* Item must be owned and non-empty. */
  assert(item->lock == worker->id);
  assert(item->count > 0);

  /* Claim destination landing. */
  if (trylock_landing(item->desc->landing, worker) == false) {
    /* Nothing can be done. */
    return false;
  }

  /* Bring item descriptors past any garbage collectable landings. */
  while (item->desc->landing->type == LAND_garbage) {
    /* Get subsequent descriptor. */
    snet_stream_desc_t *next_desc = DESC_LAND_SPEC(item->desc, siso)->outdesc;

    /* Release landing claim. */
    unlock_landing(item->desc->landing);

    /* Decrease reference counts to descriptor. */
    SNetDescRelease(item->desc, item->count);

    /* Take item out of hash table. */
    SNetHashPtrRemove(worker->hash_ptab, item->desc);

    /* Update item descriptor. */
    item->desc = next_desc;

    /* Also advance past subsequent garbage landings. */
    while (next_desc->landing->type == LAND_garbage) {

      /* Get subsequent descriptor. */
      item->desc = DESC_LAND_SPEC(next_desc, siso)->outdesc;

      /* Test if current descriptor is also in our hash table. */
      lookup = (work_item_t *)SNetHashPtrLookup(worker->hash_ptab, next_desc);
      if (lookup && trylock_work_item(lookup, worker)) {
        /* Merge both descriptor counts into one. */
        item->count += lookup->count;
        lookup->count = 0;
        unlock_work_item(lookup, worker);
      }

      /* Decrease reference counts to garbage descriptor. */
      SNetDescRelease(next_desc, item->count);

      /* Advance to subsequent descriptor and repeat. */
      next_desc = item->desc;
    }

    /* The new descriptor may already exist in hash table. */
    lookup = (work_item_t *)SNetHashPtrLookup(worker->hash_ptab, next_desc);
    if (lookup) {
      /* Merge the two counts. */
      AAF(&lookup->count, item->count);
      /* Reset item. */
      item->count = 0;
      /* We already have this desciptor in lookup, so reset it. */
      item->desc = NULL;
      /* We made progress. */
      return true;
    }
    else /* (lookup == NULL) */ {

      /* Add new descriptor to hash table. */
      SNetHashPtrStore(worker->hash_ptab, item->desc, item);

      /* Claim destination landing. */
      if (trylock_landing(item->desc->landing, worker) == false) {
        /* We made progress anyway. */
        return true;
      }
    }
  }

  /* Subtract one read license. */
  --item->count;

  /* Unlock item so thieves can steal it while we work. */
  unlock_work_item(item, worker);

  /* Finally, do the work by delegating to the streams layer. */
  SNetStreamWork(item->desc, worker);

  /* We definitely made progress. */
  return true;
}

static bool SNetInputAllowed(worker_t *worker)
{
  bool allowed = true;

  if (SNetInputThrottle()) {
    /* Check if output has advanced enough for us to input more. */
    size_t outs = NODE_SPEC(worker->output_node, output)->num_outputs;
    size_t ins = DESC_LAND_SPEC(worker->input_desc, input)->num_inputs;
    double limit = SNetInputOffset() + SNetInputFactor() * outs;
    allowed = (ins < limit);
  }

  return allowed;
}

/* Try to read next input record: true iff successful. */
static bool SNetWorkerInput(worker_t *worker)
{
  if (worker->has_input) {
    landing_t           *land = worker->input_desc->landing;

    if (trylock_landing(land, worker)) {
      if (SNetInputAllowed(worker)) {
        input_state_t      state = SNetGetNextInputRecord(land);

        if (state != INPUT_reading) {
          worker->has_input = false;
          if (state == INPUT_terminating) {
            SNetCloseInput(land->node);
          }
        }
      }
      unlock_landing(land);
    }
  }

  return worker->has_input;
}

/* Process work items from the to-do list. */
static bool SNetWorkerWork(worker_t *worker)
{
  bool  didwork = true;

  trace(__func__);

  /* Loop until no more work could be done. */
  while (didwork) {
    didwork = false;

    /* Initialize iterator to the head of the to-do list. */
    worker->prev = &worker->todo.head;
    worker->iter = worker->prev->next_item;

    /* Traverse the to-do list until work is done. */
    while (!didwork && worker->iter) {
      work_item_t *item = worker->iter;

      /* Lock the work item */
      if (trylock_work_item(item, worker)) {

        /* Test if work to be done. */
        if (item->count > 0 && SNetWorkerWorkItem(item, worker)) {
          didwork = true;
        }

        /* Remove empty items. */
        if (item->count == 0) {
          if (item->lock == worker->id || trylock_work_item(item, worker)) {
            if (item->desc) {
              SNetHashPtrRemove(worker->hash_ptab, item->desc);
            }
            worker->prev->next_item = item->next_item;
            PutFreeWorkItem(worker, item);
            item = worker->prev;
          }
        }
        else if (item->lock == worker->id) {
          unlock_work_item(item, worker);
        }
      }

      /* Advance iterator by one step. */
      worker->prev = item;
      worker->iter = item->next_item;
    }
  }

  worker->has_work = (worker->todo.head.next_item != NULL);
  return worker->has_work;
}

/* Steal a work item from another worker */
void SNetWorkerStealVictim(worker_t *victim, worker_t *thief)
{
  work_item_t   *item = victim->todo.head.next_item;
  int            amount;

  assert(thief->loot.desc == NULL);
  for (; item && !thief->loot.desc; item = item->next_item) {
    if (item->count > 0 && trylock_work_item(item, thief)) {
      if (item->count > 0 && item->desc && DESC_LOCK(item->desc) == 0) {
        work_item_t *lookup = (work_item_t *)
            SNetHashPtrLookup(thief->hash_ptab, item->desc);
        if (lookup) {
          if (item->desc->landing->type == LAND_garbage) {
            /* Take everything. */
            amount = item->count;
          } else {
            /* Split the work evenly. */
            amount = (item->count - lookup->count) / 2;
          }
          if (amount > 0) {
            FAS(&item->count, amount);
            thief->loot.count = amount;
          } else {
            thief->loot.count = 0;
          }
          thief->loot.desc = item->desc;
          thief->loot.item = lookup;
          thief->has_work = true;
        }
        else /* (lookup == NULL) */ {
          if (item->desc->landing->type == LAND_garbage) {
            /* Take everything. */
            amount = item->count;
          } else {
            /* Talk half of it, but at least one. */
            amount = (item->count + 1) / 2;
          }
          if (amount > 0) {
            FAS(&item->count, amount);
            thief->loot.count = amount;
            thief->loot.desc = item->desc;
            thief->loot.item = NULL;
            thief->has_work = true;
          }
        }
      }
      unlock_work_item(item, thief);
    }
  }
  if (thief->loot.desc && SNetDebugWS()) {
    printf("steal\n");
  }
}

/* Scan other workers for stealable items */
static bool SNetWorkerSteal(worker_t *thief)
{
  int           i;

  assert(thief->loot.desc == NULL);
  assert(thief->loot.item == NULL);

  for (i = 0; i < snet_worker_count && !thief->loot.desc; ++i) {
    thief->victim_id = 1 + (thief->victim_id % snet_worker_count);
    if (thief->victim_id != thief->id) {
      worker_t *victim = snet_workers[thief->victim_id];
      if (victim && trylock_worker(victim, thief)) {
        AAF(&victim->steal_turn->turn, 1);
        SNetWorkerStealVictim(victim, thief);
        AAF(&victim->steal_turn->turn, 1);
        unlock_worker(victim, thief);
      }
    }
  }

  return (thief->loot.desc != NULL);
}

/* Process a stolen item. */
static void SNetWorkerLoot(worker_t *worker)
{
  if (worker->loot.desc) {
    if (worker->loot.item) {
      if (worker->loot.count > 0) {
        AAF(&(worker->loot.item->count), worker->loot.count);
      }
      if (trylock_work_item(worker->loot.item, worker)) {
        if (worker->loot.item->count > 0) {
          SNetWorkerWorkItem(worker->loot.item, worker);
        }
        if (worker->loot.item->lock == worker->id) {
          unlock_work_item(worker->loot.item, worker);
        }
      }
    }
    else /* (worker->loot.item == NULL) */ {
      work_item_t *item = GetFreeWorkItem(worker);
      item->next_item = NULL;
      item->next_free = NULL;
      item->desc = worker->loot.desc;
      item->lock = worker->id;
      item->count = worker->loot.count;
      SNetHashPtrStore(worker->hash_ptab, item->desc, item);
      SNetWorkerWorkItem(item, worker);
      if (item->count == 0) {
        if (item->desc) {
          SNetHashPtrRemove(worker->hash_ptab, item->desc);
        }
        item->lock = worker->id;
        PutFreeWorkItem(worker, item);
      } else {
        item->lock = 0;
        item->next_item = worker->prev->next_item;
        BAR();
        worker->prev->next_item = worker->iter = item;
      }
    }
    worker->loot.desc = NULL;
    worker->loot.count = 0;
    worker->loot.item = NULL;
    worker->has_work = (worker->todo.head.next_item != NULL);
  }
}

/* Return the worker which is the input manager for Distributed S-Net. */
worker_t* SNetWorkerGetInputManager(void)
{
  int   i;

  for (i = snet_worker_count; i > 0; --i) {
    worker_t *worker = snet_workers[i];
    if (worker->role == InputManager) {
      return worker;
    }
  }
  return NULL;
}

/* Test if other workers have work to do. */
bool SNetWorkerOthersBusy(worker_t *worker)
{
  int   i;
  bool  change = false;

  if (worker->output_node) {
    if (NODE_SPEC(worker->output_node, output)->terminated) {
      return false;
    }
  }

  if (worker->is_idle < 1) {
    worker->idle_seqnr += 1;
    worker->is_idle = 1;
  }

  for (i = 1; i <= snet_worker_count; ++i) {
    worker_t *other = snet_workers[i];
    if (!other) {
      worker->is_idle = 1;
      change = true;
      break;
    }
    else if (other != worker) {
      if (other->is_idle == 0) {
        worker->is_idle = 1;
        change = true;
        break;
      }
      else if (worker->idle_seqnr < other->idle_seqnr) {
        worker->idle_seqnr = other->idle_seqnr;
        worker->is_idle = 1;
        change = true;
      }
      else if (worker->idle_seqnr > other->idle_seqnr) {
        worker->is_idle = 1;
        change = true;
      }
      else if (other->is_idle < worker->is_idle) {
        change = true;
      }
    }
  }
  if (change == false) {
    worker->is_idle += 1;
  }

  return (worker->is_idle < 3);
}

/* Cleanup unused memory. */
void SNetWorkerMaintenaince(worker_t *worker)
{
  /* Initialize iterator to the head of the to-do list. */
  worker->prev = &worker->todo.head;
  worker->iter = worker->prev->next_item;

  /* Traverse the to-do list. */
  while (worker->iter) {
    work_item_t *item = worker->iter;

    /* Look for empty items. */
    if (item->count == 0) {

      /* Lock the work item */
      if (trylock_work_item(item, worker)) {

        /* Remove item from hash table. */
        if (item->desc) {
          SNetHashPtrRemove(worker->hash_ptab, item->desc);
        }

        /* Delete item from list. */
        worker->prev->next_item = item->next_item;

        /* Thieves may still be accessing this, so put on free list. */
        PutFreeWorkItem(worker, item);

        /* Restore validity of item pointer. */
        item = worker->prev;
      }
    }

    /* Advance iterator by one step. */
    worker->prev = item;
    worker->iter = item->next_item;
  }

  /* Signal thieves about potential work. */
  worker->has_work = (worker->todo.head.next_item != NULL);

  /* Reduce length of free item list. */
  ReduceWorkItems(worker);
}

/* Wait for other workers to finish. */
void SNetWorkerWait(worker_t *worker)
{
  while (SNetWorkerOthersBusy(worker)) {
    SNetWorkerMaintenaince(worker);
    usleep(1000);
  }
}

/* Process work forever and read input until EOF. */
void SNetWorkerRun(worker_t *worker)
{
  useconds_t            udelay;
  // const useconds_t      max_udelay = 100;

  trace(__func__);

  for (;;) {
    if (worker->has_work || worker->has_input) {
      udelay = 10;
      do {
        if (!SNetWorkerInput(worker)) {
          sched_yield();
          if (!SNetWorkerSteal(worker) && !worker->has_work) {
            usleep(udelay);
            /* if ((udelay *= 2) > max_udelay) {
              udelay = max_udelay;
            } */
          }
          else if (worker->loot.desc) {
            SNetWorkerLoot(worker);
          }
        }
      } while (!worker->has_work && worker->has_input);
      if (worker->has_work) {
        SNetWorkerWork(worker);
      }
    }
    else {
      udelay = 10;
      do {
        sched_yield();
        if (snet_thief_limit) {
          LOCK(snet_idle_lock);
        }
        if (SNetWorkerSteal(worker)) {
          if (snet_thief_limit) {
            UNLOCK(snet_idle_lock);
          }
          break;
        }
        if (!worker->has_work) {
          usleep(udelay);
          /* if ((udelay *= 2) > max_udelay) {
            udelay = max_udelay;
          } */
        }
        if (snet_thief_limit) {
          UNLOCK(snet_idle_lock);
        }
      } while (!worker->has_work && SNetWorkerOthersBusy(worker));
      if (worker->loot.desc) {
        worker->is_idle = false;
        SNetWorkerLoot(worker);
        SNetWorkerWork(worker);
      }
      else if (worker->has_work) {
        worker->is_idle = false;
        SNetWorkerWork(worker);
      }
      else {
        break;
      }
    }
  }
  ReduceWorkItems(worker);
  if (SNetVerbose()) {
    printf("Worker %2d done\n", worker->id);
  }
}

