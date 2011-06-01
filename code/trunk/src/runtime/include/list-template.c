#include <stdarg.h>
#include <string.h>

#include "memfun.h"

#ifndef LIST_NAME_C
#error List requires a LIST_NAME_C value to be defined.
#endif

#ifndef LIST_TYPE_NAME_C
#error List requires a LIST_TYPE_NAME_C value to be defined.
#endif

#ifndef LIST_VAL_C
#error List requires a LIST_VAL_C value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using CONCAT or LIST_FUNCTION/LIST directly using ##
 * results in behaviour like "LIST(LIST_NAME_C)" expanding to
 * "snet_LIST_NAME_C_list_t" instead of containing the *value* of LIST_NAME_C.
 *
 * 2) These macros are partially duplicated in list-template.h, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define LIST(name)                      CONCAT(snet_, name, _list, _t)
#define LIST_FUNCTION(name, funName)    CONCAT(SNet, name, List, funName)
#define snet_list_t                     LIST(LIST_TYPE_NAME_C)

snet_list_t *LIST_FUNCTION(LIST_NAME_C, Create)(int size, ...)
{
  int i;
  va_list args;
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = size;
  result->used = size;
  result->values = SNetMemAlloc(size * sizeof(LIST_VAL_C));

  va_start(args, size);
  for (i = 0; i < size; i++) {
    result->values[i] = va_arg(args, LIST_VAL_C);
  }
  va_end(args);

  return result;
}

snet_list_t *LIST_FUNCTION(LIST_NAME_C, Copy)(snet_list_t *list)
{
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = list->used;
  result->used = list->used;

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL_C));
  memcpy(result->values, list->values, list->used * sizeof(LIST_VAL_C));

  return result;
}

snet_list_t *LIST_FUNCTION(LIST_NAME_C, DeepCopy)(snet_list_t *list, LIST_VAL_C (*copyfun)(LIST_VAL_C))
{
  int i;
  snet_list_t *result = SNetMemAlloc(sizeof(snet_list_t));

  result->size = list->used;
  result->used = list->used;

  result->values = SNetMemAlloc(list->used * sizeof(LIST_VAL_C));
  for (i = 0; i < result->used; i++) {
    result->values[i] = (*copyfun)(list->values[i]);
  }

  return result;
}

void LIST_FUNCTION(LIST_NAME_C, Destroy)(snet_list_t *list)
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



int LIST_FUNCTION(LIST_NAME_C, Length)(snet_list_t *list)
{
  return list->used;
}

void LIST_FUNCTION(LIST_NAME_C, Append)(snet_list_t *list, LIST_VAL_C val)
{
  if (list->used == list->size) {
    LIST_VAL_C *values = SNetMemAlloc((list->size + 1) * sizeof(LIST_VAL_C));
    memcpy(values, list->values, list->size * sizeof(LIST_VAL_C));
    SNetMemFree(list->values);

    list->values = values;
    list->size++;
  }

  list->values[list->used] = val;
  list->used++;
}

LIST_VAL_C LIST_FUNCTION(LIST_NAME_C, Pop)(snet_list_t *list)
{
  if (list->used == 0) {
    //FIXME: Crash!
  }

  list->used--;
  return list->values[list->used];
}



bool LIST_FUNCTION(LIST_NAME_C, Contains)(snet_list_t *list, LIST_VAL_C val)
{
  int i;
  for (i = 0; i < list->used; i++) {
    if (list->values[i] == val) {
      return true;
    }
  }

  return false;
}



LIST_VAL_C LIST_FUNCTION(LIST_NAME_C, Get)(snet_list_t *list, int i)
{
  if (list->used <= i) {
    //FIXME: Crash!
  }

  return list->values[i];
}

LIST_VAL_C LIST_FUNCTION(LIST_NAME_C, Remove)(snet_list_t *list, int i)
{
  LIST_VAL_C result;
  if (list->used <= i) {
    //FIXME: Crash!
  }

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
