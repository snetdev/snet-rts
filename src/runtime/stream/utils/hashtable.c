/* Hash table with 64bit keys and separate chaining */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "hashtable.h"
#include "memfun.h"

/* Calculate the number of elements in a static array. */
#define NUM_ELEMS(x)    (sizeof(x) / sizeof((x)[0]))

#define SMALLEST_PRIME  53

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

typedef struct hashtable_entry {
  uint64_t                key;
  void                   *value;
  struct hashtable_entry *next;
} hashtable_entry_t;

struct snet_hashtable{
  hashtable_entry_t   **entries;
  int                   size;
  int                   elements;
  int                   prime;
  int                   index;
};

#define SNetHashtableHash(table, key) ((int)((key) % (table)->prime))

snet_hashtable_t *SNetHashtableCreate(int size)
{
  snet_hashtable_t *table = SNetMemAlloc(sizeof(snet_hashtable_t));
  int i;

  assert(size >= 0);
  if (size == 0) {
    size = SMALLEST_PRIME;
  }
  for (i = 0; i < num_ideal_primes; ++i) {
    if (ideal_primes[i].prime >= size || i + 1 == num_ideal_primes) {
      table->size = size = ideal_primes[i].prime;
      table->index = i;
      table->prime = ideal_primes[i].prime;
      break;
    }
  }
  table->elements = 0;
  table->entries = SNetMemAlloc(sizeof(hashtable_entry_t *) * table->size);
  memset(table->entries, 0, sizeof(snet_hashtable_t *) * table->size);

  return table;
}

void SNetHashtableDestroy(snet_hashtable_t *table)
{
  int i;

  for (i = 0; i < table->size; i++) {
    while (table->entries[i] != NULL) {
      hashtable_entry_t *next = table->entries[i];
      table->entries[i] = next->next;
      SNetMemFree(next);
    }
  }

  SNetMemFree(table->entries);
  SNetMemFree(table);
}

int SNetHashtableSize(snet_hashtable_t *table)
{
  return table->elements;
}

void *SNetHashtableGet(snet_hashtable_t *table, uint64_t key)
{
  if (table->elements > 0) {
    int index = SNetHashtableHash(table, key);
    hashtable_entry_t *temp;
    for (temp = table->entries[index]; temp; temp = temp->next) {
      if (temp->key == key) {
        return temp->value;
      }
    }
  }

  return NULL;
}

int SNetHashtablePut(snet_hashtable_t *table, uint64_t key, void *value)
{
  int index = SNetHashtableHash(table, key);
  hashtable_entry_t *temp = table->entries[index];

  for (; temp; temp = temp->next) {
    if (temp->key == key) {
      assert(false);
      return SNET_HASHTABLE_ERROR;
    }
  }

  temp = SNetMemAlloc(sizeof(hashtable_entry_t));
  temp->key = key;
  temp->value = value;
  temp->next = table->entries[index];
  table->entries[index] = temp;

  table->elements++;

  return SNET_HASHTABLE_SUCCESS;
}

void *SNetHashtableReplace(snet_hashtable_t *table, uint64_t key, void *new_value)
{
  int index = SNetHashtableHash(table, key);
  hashtable_entry_t *temp = table->entries[index];

  for (; temp; temp = temp->next) {
    if (temp->key == key) {
      void *value = temp->value;
      temp->value = new_value;
      return value;
    }
  }

  return NULL;
}

void *SNetHashtableRemove(snet_hashtable_t *table, uint64_t key)
{
  int index = SNetHashtableHash(table, key);
  hashtable_entry_t *temp = table->entries[index];
  hashtable_entry_t *last = NULL;
  void *value = NULL;

  while (temp && temp->key != key) {
    last = temp;
    temp = temp->next;
  }
  if (temp) {
    value = temp->value;
    if (last) {
      last->next = temp->next;
    } else {
      table->entries[index] = temp->next;
    }
    SNetMemFree(temp);
    table->elements--;
  }

  return value;
}

bool SNetHashtableFirstKey(snet_hashtable_t *table, uint64_t *first)
{
  bool found = false;
  int i;
  for (i = 0; i < table->size; ++i) {
    if (table->entries[i]) {
      *first = table->entries[i]->key;
      found = true;
    }
  }
  return found;
}

bool SNetHashtableNextKey(snet_hashtable_t *table, uint64_t key, uint64_t *next)
{
  bool                  found = false;
  int                   index = SNetHashtableHash(table, key);
  hashtable_entry_t    *temp;

  for (temp = table->entries[index]; temp; temp = temp->next) {
    if (temp->key == key) {
      break;
    }
  }
  if (temp) {
    if (temp->next) {
      *next = temp->next->key;
      found = true;
    } else {
      while (++index < table->size) {
        if (table->entries[index]) {
          *next = table->entries[index]->key;
          found = true;
        }
      }
    }
  }

  return found;
}

