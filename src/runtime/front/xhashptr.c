#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "bool.h"
#include "memfun.h"

/* Calculate the number of elements in a static array. */
#define NUM_ELEMS(x)    (sizeof(x) / sizeof((x)[0]))

/* Limit a scalar to a bounded range. */
#define LIMIT(x,l,u)    ((x) < (l) ? (l) : (x) > (u) ? (u) : (x))

/* First power in the ideal primes table. */
#define FIRST_POWER     5

/* Last power in the ideal primes table. */
#define LAST_POWER      30

/* Hash a pointer to a table index. */
#define HASH_PTR(p,n)   ((hash_t) (p) % (n))

/* The type of a hashed pointer. */
typedef uintptr_t hash_t;

/* Ideal primes for division hashing from planetmath.org/goodhashtableprimes */
static struct ideal_primes {
  size_t power;
  size_t prime;
} ideal_primes[] = {
  {  5, 53 },
  {  6, 97 },
  {  7, 193 },
  {  8, 389 },
  {  9, 769 },
  { 10, 1543 },
  { 11, 3079 },
  { 12, 6151 },
  { 13, 12289 },
  { 14, 24593 },
  { 15, 49157 },
  { 16, 98317 },
  { 17, 196613 },
  { 18, 393241 },
  { 19, 786433 },
  { 20, 1572869 },
  { 21, 3145739 },
  { 22, 6291469 },
  { 23, 12582917 },
  { 24, 25165843 },
  { 25, 50331653 },
  { 26, 100663319 },
  { 27, 201326611 },
  { 28, 402653189 },
  { 29, 805306457 },
  { 30, 1610612741 },
};
static const size_t num_ideal_primes = NUM_ELEMS(ideal_primes);

/* A hashed node stores a keyword/value pair. */
typedef struct hash_node {
  struct hash_node      *next;
  void                  *key;
  void                  *val;
} hash_node_t;

/* Pointer hash table stores hash nodes and a free list.
 * The table is a big array of small lists.
 * When auto_resize is true grow table when capacity is reached. */
typedef struct hash_ptab {
  size_t        power;
  size_t        prime;
  size_t        count;
  size_t        freed;
  bool          auto_resize;
  hash_node_t  *free;
  hash_node_t **tab;
} hash_ptab_t;

/* Create a new hash table for pointer hashing of 2^power size. */
struct hash_ptab* SNetHashPtrTabCreate(size_t power, bool auto_resize)
{
  size_t        prime;
  hash_ptab_t  *tab;
  size_t        i;

  power = LIMIT(power, FIRST_POWER, LAST_POWER);
  prime = ideal_primes[power - FIRST_POWER].prime;
  tab = SNetNew(hash_ptab_t);
  tab->tab = SNetNewN(prime, hash_node_t *);

  tab->power = power;
  tab->prime = prime;
  tab->count = 0;
  tab->freed = 0;
  tab->auto_resize = auto_resize;
  tab->free = NULL;
  for (i = 0; i < prime; ++i) {
    tab->tab[i] = NULL;
  }
  return tab;
}

/* Destroy a hash table for pointer lookup. */
void SNetHashPtrTabDestroy(struct hash_ptab *tab)
{
  size_t        i;
  hash_node_t   *node, *next;

  /* Loop over table. */
  for (i = 0; i < tab->prime; ++i) {
    /* Destroy each list. */
    for (node = tab->tab[i]; node; node = next) {
      next = node->next;
      SNetDelete(node);
    }
  }
  /* Destroy free list. */
  for (node = tab->free; node; node = next) {
    next = node->next;
    SNetDelete(node);
  }
  SNetDelete(tab->tab);
  SNetDelete(tab);
}

/* Resize a pointer hash table to a new power size. */
void SNetHashPtrTabResize(struct hash_ptab *tab, size_t new_power)
{
  new_power = LIMIT(new_power, FIRST_POWER, LAST_POWER);

  if (new_power != tab->power) {
    /* Allocate a new table. */
    size_t new_prime = ideal_primes[new_power - FIRST_POWER].prime;
    hash_node_t **new_tab = SNetNewN(new_prime, hash_node_t *);
    size_t i;
    for (i = 0; i < new_prime; ++i) {
      new_tab[i] = NULL;
    }
    /* Loop over old table and rehash nodes into the new table. */
    for (i = 0; i < tab->prime; ++i) {
      hash_node_t   *node, *next;
      for (node = tab->tab[i]; node; node = next) {
        hash_t hash = HASH_PTR(node->key, new_prime);
        next = node->next;
        node->next = new_tab[hash];
        new_tab[hash] = node;
      }
    }
    /* Delete old table. */
    SNetDeleteN(tab->prime, tab->tab);
    /* Store new configuration into table. */
    tab->tab = new_tab;
    tab->prime = new_prime;
    tab->power = new_power;
  }
}

void *SNetHashPtrLookup(struct hash_ptab *tab, void *key);

/* Store a pointer + value pair into a pointer hashing table. */
void SNetHashPtrStore(struct hash_ptab *tab, void *key, void *val)
{
  hash_t        hash = HASH_PTR(key, tab->prime);
  hash_node_t  *node;

  assert(tab->tab[hash] == NULL || tab->tab[hash]->key != key);
  
  /* Obtain a new or unused node. */
  if (tab->free) {
    node = tab->free;
    tab->free = node->next;
    --tab->freed;
  } else {
    node = SNetNew(hash_node_t);
  }

  /* Store keyword/value and add to table. */
  node->key = key;
  node->val = val;
  node->next = tab->tab[hash];
  tab->tab[hash] = node;
  ++tab->count;

  /* Resize when capacity is reached. */
  if (tab->auto_resize && tab->count > (2 << tab->power)) {
    SNetHashPtrTabResize(tab, 3 + tab->power);
  }
}

/* Lookup a value in the pointer hashing table. */
void *SNetHashPtrLookup(struct hash_ptab *tab, void *key)
{
  hash_t        hash = HASH_PTR(key, tab->prime);
  hash_node_t  *node;

  for (node = tab->tab[hash]; node; node = node->next) {
    if (node->key == key) {
      return node->val;
    }
  }
  return NULL;
}

/* Remove a value from the pointer hashing table. */
void *SNetHashPtrRemove(struct hash_ptab *tab, void *key)
{
  hash_t         hash = HASH_PTR(key, tab->prime);
  hash_node_t   *node = tab->tab[hash];
  hash_node_t   *prev = NULL;
  void          *val = NULL;

  /* Lookup node which stores keyword. */
  while (node && node->key != key) {
    prev = node;
    node = node->next;
  }
  /* Remove node from table. */
  if (node) {
    val = node->val;
    if (prev) {
      prev->next = node->next;
    } else {
      tab->tab[hash] = node->next;
    }
    node->next = tab->free;
    tab->free = node;
    ++tab->freed;
    --tab->count;

    /* Retain between 12% and 25% of free items. */
    if (4 * tab->freed > tab->prime) {
      size_t limit = tab->prime / 8;
      while (tab->free && tab->freed > limit) {
        node = tab->free;
        tab->free = node->next;
        SNetDelete(node);
        --tab->freed;
      }
    }
  } else {
    assert(false);
  }
  return val;
}

/* Verify whether the table has zero elements. */
bool SNetHashPtrTabEmpty(struct hash_ptab *tab)
{
  bool empty = true;
  size_t i;
  for (i = 0; i < tab->prime; ++i) {
    if (tab->tab[i]) {
      empty = false;
      break;
    }
  }
  return empty;
}

/* Return the first key in the hash table. */
void *SNetHashPtrFirst(struct hash_ptab *tab)
{
  void *first = NULL;
  size_t i;
  for (i = 0; i < tab->prime; ++i) {
    if (tab->tab[i]) {
      first = tab->tab[i]->key;
      break;
    }
  }
  return first;
}

/* Return the next key in the hash table after 'key'. */
void *SNetHashPtrNext(struct hash_ptab *tab, void *key)
{
  hash_t        hash = HASH_PTR(key, tab->prime);
  hash_node_t  *node = tab->tab[hash];
  size_t        i;
  void         *next = NULL;

  while (node && node->key != key) {
    node = node->next;
  }
  assert(node);
  if (node->next) {
    next = node->next->key;
  } else {
    for (i = 1 + (size_t) hash; i < tab->prime; ++i) {
      if (tab->tab[i]) {
        next = tab->tab[i]->key;
        break;
      }
    }
  }
  return next;
}

