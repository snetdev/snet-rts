#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "memfun.h"

#ifndef LIST_NAME
#error List requires a LIST_NAME value to be defined (mirroring LIST_NAME).
#endif

#ifndef LIST_TYPE_NAME
#error List requires a LIST_TYPE_NAME value to be defined (mirroring
#error LIST_TYPE_NAME).
#endif

#ifndef LIST_VAL
#error List requires a LIST_VAL value to be defined (mirroring LIST_VAL).
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using ## results in behaviour like
 * "LIST(LIST_NAME)" expanding to "snet_LIST_NAME_list_t" instead of containing
 * the *value* of LIST_NAME.
 *
 * 2) These macros are partially duplicated in list-template.h, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define LIST(name)                      CONCAT(snet_, name, _list, _t)
#define LIST_FUNCTION(name, funName)    CONCAT(SNet, name, List, funName)
#define snet_list_t                     LIST(LIST_TYPE_NAME)

snet_list_t *LIST_FUNCTION(LIST_NAME, Create)(int size, ...)
{
  int i;
  va_list args;
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = size;
  result->used = size;
  result->start = 0;
  result->values = SNetMemAlloc(size * sizeof(LIST_VAL));

  va_start(args, size);
  for (i = 0; i < size; i++) {
    result->values[i] = va_arg(args, LIST_VAL);
  }
  va_end(args);

  return result;
}

snet_list_t *LIST_FUNCTION(LIST_NAME, Copy)(snet_list_t *list)
{
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = list->used;
  result->used = list->used;
  result->start = 0;

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL));

  int startToArrayEnd = list->size - list->start;
  memcpy(result->values, list->values + list->start,
         startToArrayEnd * sizeof(LIST_VAL));
  memcpy(result->values + startToArrayEnd, list->values,
         (list->size - startToArrayEnd) * sizeof(LIST_VAL));

  return result;
}

void LIST_FUNCTION(LIST_NAME, Destroy)(snet_list_t *list)
{
  #ifdef LIST_FREE_FUNCTION
  LIST_VAL val;

  LIST_FOR_EACH(list, val)
    LIST_FREE_FUNCTION(val);
  END_FOR
  #endif

  SNetMemFree(list->values);
  SNetMemFree(list);
}



snet_list_t *LIST_FUNCTION(LIST_NAME, DeepCopy)(snet_list_t *list,
                                                LIST_VAL (*copyfun)(LIST_VAL))
{
  int i;
  LIST_VAL val;
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = list->used;
  result->used = list->used;
  result->start = 0;

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL));
  LIST_ENUMERATE(list, val, i)
    result->values[i] = (*copyfun)(val);
  END_ENUMERATE

  return result;
}



int LIST_FUNCTION(LIST_NAME, Length)(snet_list_t *list)
{
  return list->used;
}



void LIST_FUNCTION(LIST_NAME, Push)(snet_list_t *list, LIST_VAL val)
{
  if (list->used == list->size) {
    LIST_VAL *values = SNetMemAlloc((list->size + 1) * sizeof(LIST_VAL));

    int startToArrayEnd = list->size - list->start;
    memcpy(values, list->values + list->start,
           startToArrayEnd * sizeof(LIST_VAL));
    memcpy(values + startToArrayEnd, list->values,
           (list->size - startToArrayEnd) * sizeof(LIST_VAL));

    SNetMemFree(list->values);

    list->values = values;
    list->size++;
    list->start = 0;
  }

  list->values[(list->start + list->used) % list->size] = val;
  list->used++;
}

void LIST_FUNCTION(LIST_NAME, Append)(snet_list_t *list, LIST_VAL val)
{
  if (list->used == list->size) {
    LIST_VAL *values = SNetMemAlloc((list->size + 1) * sizeof(LIST_VAL));

    int startToArrayEnd = list->size - list->start;
    memcpy(values + 1, list->values + list->start,
           startToArrayEnd * sizeof(LIST_VAL));
    memcpy(values + 1 + startToArrayEnd, list->values,
           (list->size - startToArrayEnd) * sizeof(LIST_VAL));

    SNetMemFree(list->values);
    list->values = values;
    list->size++;
    list->start = 1;
  }
  //Note: first add size to start before subtracting 1 to make sure start is
  //always >= 0
  list->start = (list->start + list->size - 1) % list->size;
  list->values[list->start] = val;
  list->used++;
}

LIST_VAL LIST_FUNCTION(LIST_NAME, Pop)(snet_list_t *list)
{
  assert(list->used > 0); //FIXME: Desired behaviour?

  list->used--;
  return list->values[(list->start + list->used) % list->size];
}

LIST_VAL LIST_FUNCTION(LIST_NAME, Unappend)(snet_list_t *list)
{
  assert(list->used > 0); //FIXME: Desired behaviour?

  list->used--;
  list->start++;
  return list->values[list->start - 1];
}



bool LIST_FUNCTION(LIST_NAME, Contains)(snet_list_t *list, LIST_VAL val)
{
  LIST_VAL tmp;

  LIST_FOR_EACH(list, tmp)
    #ifdef LIST_CMP
      if (LIST_CMP(tmp, val)) {
        return true;
      }
    #else
      if (tmp == val) {
        return true;
      }
    #endif
  END_FOR

  return false;
}



LIST_VAL LIST_FUNCTION(LIST_NAME, Get)(snet_list_t *list, int i)
{
  assert(i >= 0 && list->used > i); //FIXME: Desired behaviour?

  return list->values[(list->start + i) % list->size];
}

LIST_VAL LIST_FUNCTION(LIST_NAME, Remove)(snet_list_t *list, int i)
{
  LIST_VAL result;

  assert(i >= 0 && list->used > i); //FIXME: Desired behaviour?

  result = list->values[i];

  for (; i < list->used; i++) {
    list->values[(list->start + i) % list->size] =
              list->values[(list->start + i + 1) % list->size];
  }

  list->used--;
  return result;
}




void LIST_FUNCTION(LIST_NAME, Serialise)(snet_list_t *list,
    void (*serialiseInts)(int, int*),
    void (*serialiseValues)(int, LIST_VAL*))
{
  serialiseInts(1, &list->used);
  serialiseValues(list->used, list->values);
}

void LIST_FUNCTION(LIST_NAME, Deserialise)(snet_list_t *list,
    void (*deserialiseInts)(int, int*),
    void (*deserialiseValues)(int, LIST_VAL*))
{
  deserialiseInts(1, &list->used);
  list->values = SNetMemAlloc(list->used * sizeof(LIST_VAL));
  deserialiseValues(list->used, list->values);
}



#undef snet_list_t
#undef LIST_FUNCTION
#undef LIST
#undef CONCAT
