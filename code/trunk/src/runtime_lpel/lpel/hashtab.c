/**
 * File: hashtab.c
 * Auth: Daniel Prokesch
 * Date: 2010/09/15
 * Desc:
 *
 * An open-addressed hashtable implementation using quadratic probing
 * and a hashtable size (capacity) of m = 2^n.
 * The keys are integers, the values unspecified pointers (void*).
 *
 * The hash-function simply is h(k) = k % m.
 * The probing function is h(k,i) = (h(k,i-1) + i) % m  with h(k,0) = h(k).
 * A nice property thereof is, that the values h(k,i) for i in [0,m − 1] are
 * all distinct, meaning that all buckets are probed.
 *
 * Upon a load > 0.75, which is particulary easy to compute for
 * table size >= 2^2, the capacity is doubled and each element is rehashed.
 *
 * There are functions for putting values into and retrieving values from
 * the table, no function for deleting is provided.
 */

#include <stdlib.h>
#include <assert.h>

#include "hashtab.h"


typedef struct hashtab_entry {
  int key;
  void *value;
} hashtab_entry_t;

struct hashtab {
  int capacity;
  int count;
  hashtab_entry_t *table;
};

/**
 * Create a hashtable
 *
 * @param init_cap2   the initial capacity of the hashtable, specified as
 *                    power of two (the capacity will be 2^init_cap2)
 * @pre   init_cap2 >= 2
 * @return a newly allocated hashtable
 */
hashtab_t *HashtabCreate( int init_cap2)
{
  hashtab_t *ht = (hashtab_t *) malloc( sizeof(hashtab_t));
  int i;

  ht->capacity = 1 << init_cap2;
  ht->count = 0;
  ht->table = (hashtab_entry_t *) malloc(
      ht->capacity * sizeof(hashtab_entry_t));

  for (i=0; i<ht->capacity; i++) {
    ht->table[i].key = -1;
  }
  return ht;
}

/**
 * Destroy a hashtable
 *
 * @param ht    the hashtable to desrtoy, the memory will be freed
 */
void HashtabDestroy( hashtab_t *ht)
{
  free(ht->table);
  free(ht);
}


/* assuming size is a power of two */
#define MOD_SIZE(size, key)    ((key) & ((size)-1))
/* computes the key for the i-th probe, assuming the key of the (i-1)-th probe */
#define HASH_K_I(size,key,i)  (MOD_SIZE((size),(key)+(i)))

/**
 * Get a hashtab entry to store key.
 * If key exists, the entry of that key will be returned.
 */
static hashtab_entry_t *ProbePut( hashtab_t *ht, int key)
{
  int pos, i;
  hashtab_entry_t *res = NULL;

  /* find the position in the table through quadratic probing */
  pos = key;
  for (i=0; i<ht->capacity; i++) {
    pos = HASH_K_I( ht->capacity, pos, i);
    if (ht->table[pos].key == -1 ||
        ht->table[pos].key == key) {
      res = &ht->table[pos];
      break;
    }
  }
  /* every element is probed due to the nature of the
     quadratic probing hash function and 2^n capacity */
  return res;
}


/**
 * Put a key-value pair into the hashtable.
 * If key already exists, the value will be overwritten.
 *
 * @param ht    pointer to the hashtable
 * @param key   key >= 0
 * @param value value != NULL
 * @pre   (key >= 0) && (value != NULL)
 */
void HashtabPut( hashtab_t *ht, int key, void *value)
{
  hashtab_entry_t *hte;

  ht->count += 1;
  /* a load factor of 75% is used */
  if (ht->count > (ht->capacity - (ht->capacity >> 2))) {
    /* resize and rehash*/
    int i;
    int oldcap = ht->capacity;
    hashtab_entry_t *oldtab = ht->table;

    /* create a new table */
    ht->capacity = oldcap << 1; /* double */
    ht->table = (hashtab_entry_t *) malloc(
        ht->capacity * sizeof(hashtab_entry_t));
    for (i=0; i<ht->capacity; i++) {
      ht->table[i].key = -1;
    }
    /* fill the old entries */
    for (i=0; i<oldcap; i++) {
      int key = oldtab[i].key;
      if (key != -1) {
        /* get a pointer to the table entry */
        hte = ProbePut( ht, key);
        assert( hte != NULL);
        /* put the item in the table */
        hte->key   = key;
        hte->value = oldtab[i].value;
      }
    }
    /* free the old table */
    free( oldtab);
  }

  /* get a pointer to the table entry */
  hte = ProbePut( ht, key);
  assert( hte != NULL);
  /* put the item in the table */
  hte->key   = key;
  hte->value = value;
}


/**
 * Get value with specific key from the hashtable.
 * If key does not exist, NULL will be returned.
 *
 * @param ht  pointer to hashtable
 * @param key key for which the value should be retrieved
 * @return  the value stored for the key, or NULL if key does not exist
 */
void *HashtabGet( hashtab_t *ht, int key)
{
  int pos, i;
  void *res = NULL;

  /* find the position in the table through quadratic probing */
  pos = key;
  for (i=0; i<ht->capacity; i++) {
    pos = HASH_K_I( ht->capacity, pos, i);
    if (ht->table[pos].key == -1) {
      break;
    }
    if (ht->table[pos].key == key) {
      res = ht->table[pos].value;
      break;
    }
  }
  return res;
}

#ifdef DEBUG
/**
 * Print the contents of the hashtable to a file,
 * for debugging purposes.
 *
 * @param ht    pointer to hashtable
 * @param outf  file to which the contents are written to
 * @pre   file must be open for writing
 */
void HashtabPrintDebug( hashtab_t *ht, FILE *outf)
{
  int i;

  fprintf( outf, "Hashtab capacity %d, count %d\n", ht->capacity, ht->count);
  for (i=0; i<ht->capacity; i++) {
    hashtab_entry_t *hte = &ht->table[i];
    fprintf( outf, "[%3d] key %4d, value %p\n", i, hte->key, hte->value);
  }
}
#endif
