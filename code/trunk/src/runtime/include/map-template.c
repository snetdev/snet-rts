#include <stdarg.h>
#include <string.h>

#include "memfun.h"

#ifndef MAP_NAME_C
#error Map requires a MAP_NAME_C value to be defined.
#endif

#ifndef MAP_VAL_C
#error Map requires a MAP_VAL_C value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using CONCAT or MAP_FUNCTION/MAP directly using ##
 * results in behaviour like "MAP(MAP_NAME_C)" expanding to "snet_map_MAP_NAME_C_t" instead
 * of containing the *value* of MAP_NAME_C.
 *
 * 2) These macros are partially duplicated in map-template.h, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s, suffix)  prefix ## s ## suffix
#define CONCAT4(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define MAP(name)                  CONCAT(snet_, name, _map_t)
#define MAP_FUNCTION(name, funName)    CONCAT4(SNet, name, Map, funName)
#define snet_map_t                 MAP(MAP_NAME_C)

snet_map_t *MAP_FUNCTION(MAP_NAME_C, Create)(int size, MAP_VAL_C error, ...)
{
  int i;
  va_list args;
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = size;
  result->used = size;
  result->error = error;
  result->keys = SNetMemAlloc(size * sizeof(int));
  result->values = SNetMemAlloc(size * sizeof(MAP_VAL_C));

  va_start(args, error);
  for (i = 0; i < size; i++) {
    result->keys[i] = va_arg(args, int);
    result->values[i] = va_arg(args, MAP_VAL_C);
  }
  va_end(args);

  return result;
}

snet_map_t *MAP_FUNCTION(MAP_NAME_C, Copy)(snet_map_t *map)
{
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = map->used;
  result->used = map->used;

  result->keys = SNetMemAlloc(map->used * sizeof(int));
  memcpy(result->keys, map->keys, map->used * sizeof(int));

  result->values = SNetMemAlloc(map->used * sizeof(MAP_VAL_C));
  memcpy(result->values, map->values, map->used * sizeof(MAP_VAL_C));

  return result;
}

void MAP_FUNCTION(MAP_NAME_C, Destroy)(snet_map_t *map)
{
  SNetMemFree(map->keys);
  SNetMemFree(map->values);
  SNetMemFree(map);
}



static int MAP_FUNCTION(MAP_NAME_C, Find)(snet_map_t *map, int key)
{
  int i;
  for (i = 0; i < map->used; i++) {
    if (map->keys[i] == key) {
      return i;
    }
  }

  return -1;
}

int MAP_FUNCTION(MAP_NAME_C, Size)(snet_map_t *map)
{
  return map->used;
}

void MAP_FUNCTION(MAP_NAME_C, Set)(snet_map_t *map, int key, MAP_VAL_C val)
{
  int i = MAP_FUNCTION(MAP_NAME_C, Find)(map, key);

  if (i != -1) {
    map->values[i] = val;
  } else if (map->used < map->size) {
    map->keys[map->used] = key;
    map->values[map->used] = val;
    map->used++;
  } else {
    int *keys = SNetMemAlloc((map->size + 1) * sizeof(int));
    MAP_VAL_C *values = SNetMemAlloc((map->size + 1) * sizeof(MAP_VAL_C));

    memcpy(keys, map->keys, map->size * sizeof(int));
    memcpy(values, map->values, map->size * sizeof(MAP_VAL_C));

    keys[map->used] = key;
    values[map->used] = val;

    SNetMemFree(map->keys);
    SNetMemFree(map->values);

    map->keys = keys;
    map->values = values;
    map->used++;
    map->size++;
  }
}

MAP_VAL_C MAP_FUNCTION(MAP_NAME_C, Get)(snet_map_t *map, int key)
{
  int i = MAP_FUNCTION(MAP_NAME_C, Find)(map, key);

  if (i == -1) {
    return map->error;
  }

  return map->values[i];
}

MAP_VAL_C MAP_FUNCTION(MAP_NAME_C, Take)(snet_map_t *map, int key)
{
  MAP_VAL_C result;
  int i = MAP_FUNCTION(MAP_NAME_C, Find)(map, key);
  if (i == -1) {
    return map->error;
  }

  result = map->values[i];

  map->used--;
  map->keys[i] = map->keys[map->used];
  map->values[i] = map->values[map->used];

  return result;
}

void MAP_FUNCTION(MAP_NAME_C, Rename)(snet_map_t *map, int oldKey, int newKey)
{
  int i = MAP_FUNCTION(MAP_NAME_C, Find)(map, oldKey);
  if (i != -1) {
    map->keys[i] = newKey;
  }
}

#undef snet_map_t
#undef MAP_FUNCTION
#undef MAP
#undef CONCAT
