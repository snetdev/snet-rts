#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "memfun.h"

#ifndef LIST_NAME
#error List requires a LIST_NAME value to be defined (mirroring LIST_NAME_H).
#endif

#ifndef LIST_TYPE_NAME
#error List requires a LIST_TYPE_NAME value to be defined (mirroring
#error LIST_TYPE_NAME_H).
#endif

#ifndef LIST_VAL
#error List requires a LIST_VAL value to be defined (mirroring LIST_VAL_H).
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

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL));
  memcpy(result->values, list->values, list->used * sizeof(LIST_VAL));

  return result;
}

void LIST_FUNCTION(LIST_NAME, Destroy)(snet_list_t *list)
{
  #ifdef LIST_FREE_FUNCTION
  int i;
  for (i = 0; i < list->used; i++) {
    LIST_FREE_FUNCTION(list->values[i]);
  }
  #endif

  SNetMemFree(list->values);
  SNetMemFree(list);
}



snet_list_t *LIST_FUNCTION(LIST_NAME, DeepCopy)(snet_list_t *list,
                                                LIST_VAL (*copyfun)(LIST_VAL))
{
  int i;
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = list->used;
  result->used = list->used;

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL));
  for (i = 0; i < result->used; i++) {
    result->values[i] = (*copyfun)(list->values[i]);
  }

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
    memcpy(values, list->values, list->size * sizeof(LIST_VAL));
    SNetMemFree(list->values);

    list->values = values;
    list->size++;
  }

  list->values[list->used] = val;
  list->used++;
}

LIST_VAL LIST_FUNCTION(LIST_NAME, Pop)(snet_list_t *list)
{
  assert(list->used > 0); //FIXME: Desired behaviour?

  list->used--;
  return list->values[list->used];
}



bool LIST_FUNCTION(LIST_NAME, Contains)(snet_list_t *list, LIST_VAL val)
{
  int i;

  for (i = 0; i < list->used; i++) {
    if (list->values[i] == val) {
      return true;
    }
  }

  return false;
}



LIST_VAL LIST_FUNCTION(LIST_NAME, Get)(snet_list_t *list, int i)
{
  assert(i >= 0 && list->used > i); //FIXME: Desired behaviour?

  return list->values[i];
}

LIST_VAL LIST_FUNCTION(LIST_NAME, Remove)(snet_list_t *list, int i)
{
  LIST_VAL result;

  assert(i >= 0 && list->used > i); //FIXME: Desired behaviour?

  result = list->values[i];

  for (; i < list->used; i++) {
    list->values[i] = list->values[i+1];
  }

  list->used--;
  return result;
}

#undef snet_list_t
#undef LIST_FUNCTION
#undef LIST
#undef CONCAT
