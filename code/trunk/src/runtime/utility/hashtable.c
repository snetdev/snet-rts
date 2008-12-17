/* Hash table with 64bit keys and separate chaining */
#include <string.h>

#include "hashtable.h"
#include "memfun.h"

typedef struct hashtable_entry {
  uint64_t key;
  void *value;
  struct hashtable_entry *next;
} hashtable_entry_t;

struct snet_hashtable{
  snet_hashtable_compare_fun_t compare_fun;
  unsigned int size;
  unsigned int elements;
  hashtable_entry_t **entries;
};


/* TODO: Implement more efficient hash function ;-)
 *
 * although, modulo hashing may be a good approach as 
 * ids are always created by incrementing the last id by one!
 *
 */

#define SNetHashtableHash(table, key) (key % table->size)

/*
static unsigned int SNetHashtableHash(snet_hashtable_t *table, uint64_t key)
{
  //TODO:
}
*/

snet_hashtable_t *SNetHashtableCreate(int size, snet_hashtable_compare_fun_t compare_fun)
{
  snet_hashtable_t *table;

  if(size <= 0) {
    return NULL;
  } 

  table = SNetMemAlloc(sizeof(snet_hashtable_t));

  table->size = size;
  table->elements = 0;
 
  table->entries = SNetMemAlloc(sizeof(hashtable_entry_t *) * table->size);

  memset(table->entries, 0, sizeof(snet_hashtable_t *) * table->size);

  table->compare_fun = compare_fun;

  return table;
}

void SNetHashtableDestroy(snet_hashtable_t *table)
{
  int i;
  hashtable_entry_t *next;

  for(i = 0; i < table->size; i++) {
    while(table->entries[i] != NULL) {
      next = table->entries[i];

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
  unsigned int index;
  hashtable_entry_t *temp;

  if(table->elements > 0) {
    
    index = SNetHashtableHash(table, key);
    
    temp = table->entries[index];
    
    while(temp != NULL && temp->key != key) {
      temp = temp->next;
    }
    
    if(temp != NULL) {

      return temp->value;
    }
  }

  return NULL;
}

uint64_t SNetHashtableGetKey(snet_hashtable_t *table, 
			     void *value) 
{
  int i;
  hashtable_entry_t *temp;

  if(table->elements > 0) {
    for(i = 0; i < table->size; i++) {
      
      temp = table->entries[i];
      
      while(temp != NULL) {
	if(table->compare_fun(value, temp->value)) {
	return temp->key;
	}
      
	temp = temp->next;
      }
    }
  }

  return UINT64_MAX;
}

int SNetHashtablePut(snet_hashtable_t *table, uint64_t key, void *value)
{
  unsigned int index;
  hashtable_entry_t *temp;

  hashtable_entry_t *new;

  index = SNetHashtableHash(table, key);

  temp = table->entries[index];

  while(temp != NULL) {
    if(temp->key == key) {
      return SNET_HASHTABLE_ERROR;
    }

    if(temp->next == NULL) {
      break;
    } else {
      temp = temp->next;
    }
  }
  
  new = SNetMemAlloc(sizeof(hashtable_entry_t));

  new->key = key;
  new->value = value;
  new->next = NULL;

  if(temp == NULL) {
    table->entries[index] = new;
  } else {
    temp->next = new;
  }

  table->elements++;

  return SNET_HASHTABLE_SUCCESS;
}

void *SNetHashtableReplace(snet_hashtable_t *table, uint64_t key, void *new_value)
{
  unsigned int index;
  hashtable_entry_t *temp;
  hashtable_entry_t *last = NULL;
  void *value = NULL;

  index = SNetHashtableHash(table, key);

  temp = table->entries[index];

  while(temp != NULL && temp->key != key) {
    last = temp;
    temp = temp->next;
  }

  if(temp != NULL) {
    value = temp->value;

    temp->value = new_value; 
  }

  return value;
}

void *SNetHashtableRemove(snet_hashtable_t *table, uint64_t key)
{
  unsigned int index;
  hashtable_entry_t *temp;
  hashtable_entry_t *last = NULL;
  void *value = NULL;

  index = SNetHashtableHash(table, key);

  temp = table->entries[index];

  while(temp != NULL && temp->key != key) {
    last = temp;
    temp = temp->next;
  }

  if(temp != NULL) {
    value = temp->value;

    if(last != NULL) {
      last->next = temp->next;
    } else {
      table->entries[index] = temp->next;
    }

    SNetMemFree(temp);
  }

  table->elements--;

  return value;
}
