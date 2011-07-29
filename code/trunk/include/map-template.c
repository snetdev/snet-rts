#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "memfun.h"

#ifndef MAP_NAME
#error Map requires a MAP_NAME value to be defined (mirroring MAP_NAME_H).
#endif

#ifndef MAP_TYPE_NAME
#error Map requires a MAP_TYPE_NAME value to be defined (mirroring
#error MAP_TYPE_NAME_H).
#endif

#ifndef MAP_KEY
#define MAP_KEY int
#define MAP_CANARY
#endif

#ifndef MAP_VAL
#error Map requires a MAP_VAL value to be defined (mirroring MAP_VAL).
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using ## results in behaviour like
 * "MAP(MAP_NAME)" expanding to "snet_map_MAP_NAME_t" instead of containing the
 * *value* of MAP_NAME.
 *
 * 2) These macros are partially duplicated in map-template.h, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define MAP(name)                       CONCAT(snet_, name, _map, _t)
#define MAP_FUNCTION(name, funName)     CONCAT(SNet, name, Map, funName)
#define snet_map_t                      MAP(MAP_TYPE_NAME)

snet_map_t *MAP_FUNCTION(MAP_NAME, Create)(int size, ...)
{
  int i;
  va_list args;
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = size;
  result->used = size;
  result->keys = SNetMemAlloc(size * sizeof(MAP_KEY));
  result->values = SNetMemAlloc(size * sizeof(MAP_VAL));

  va_start(args, size);
  for (i = 0; i < size; i++) {
    result->keys[i] = va_arg(args, MAP_KEY);
    result->values[i] = va_arg(args, MAP_VAL);
  }
  va_end(args);

  return result;
}

snet_map_t *MAP_FUNCTION(MAP_NAME, Copy)(snet_map_t *map)
{
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = map->used;
  result->used = map->used;

  result->keys = SNetMemAlloc(map->used * sizeof(MAP_KEY));
  memcpy(result->keys, map->keys, map->used * sizeof(MAP_KEY));

  result->values = SNetMemAlloc(map->used * sizeof(MAP_VAL));
  memcpy(result->values, map->values, map->used * sizeof(MAP_VAL));

  return result;
}

void MAP_FUNCTION(MAP_NAME, Destroy)(snet_map_t *map)
{
  #ifdef MAP_KEY_FREE_FUNCTION
    int i;
  #endif
  #ifdef MAP_VAL_FREE_FUNCTION
    int j;
  #endif

  #ifdef MAP_KEY_FREE_FUNCTION
    for (i = 0; i < map->used; i++) {
      MAP_KEY_FREE_FUNCTION(map->keys[i]);
    }
  #endif
  #ifdef MAP_VAL_FREE_FUNCTION
    for (j = 0; j < map->used; j++) {
      MAP_VAL_FREE_FUNCTION(map->values[j]);
    }
  #endif

  SNetMemFree(map->keys);
  SNetMemFree(map->values);
  SNetMemFree(map);
}



snet_map_t *MAP_FUNCTION(MAP_NAME, DeepCopy)(snet_map_t *map,
                                          #ifndef MAP_CANARY
                                             MAP_KEY (*keyCopyFun)(MAP_KEY),
                                          #endif
                                             MAP_VAL (*valCopyFun)(MAP_VAL))
{
  int i;
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = map->used;
  result->used = map->used;

  result->keys = SNetMemAlloc(map->used * sizeof(MAP_KEY));
  result->values = SNetMemAlloc(map->used * sizeof(MAP_VAL));

  for (i = 0; i < result->used; i++) {
    #ifndef MAP_CANARY
      result->keys[i] = (*keyCopyFun)(map->keys[i]);
    #else
      result->keys[i] = map->keys[i];
    #endif
    result->values[i] = (*valCopyFun)(map->values[i]);
  }

  return result;
}



int MAP_FUNCTION(MAP_NAME, Size)(snet_map_t *map)
{
  return map->used;
}



static int MAP_FUNCTION(MAP_NAME, Find)(snet_map_t *map, MAP_KEY key)
{
  int i;
  for (i = 0; i < map->used; i++) {
    #ifdef MAP_KEY_CMP
      if (MAP_KEY_CMP(map->keys[i], key)) {
        return i;
      }
    #else
      if (map->keys[i] == key) {
        return i;
      }
    #endif
  }

  return -1;
}

void MAP_FUNCTION(MAP_NAME, Set)(snet_map_t *map, MAP_KEY key, MAP_VAL val)
{
  int i = MAP_FUNCTION(MAP_NAME, Find)(map, key);

  if (i != -1) {
    map->values[i] = val;
  } else if (map->used < map->size) {
    map->keys[map->used] = key;
    map->values[map->used] = val;
    map->used++;
  } else {
    MAP_KEY *keys = SNetMemAlloc((map->size + 1) * sizeof(MAP_KEY));
    MAP_VAL *values = SNetMemAlloc((map->size + 1) * sizeof(MAP_VAL));

    memcpy(keys, map->keys, map->size * sizeof(MAP_KEY));
    memcpy(values, map->values, map->size * sizeof(MAP_VAL));

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

MAP_VAL MAP_FUNCTION(MAP_NAME, Get)(snet_map_t *map, MAP_KEY key)
{
  int i = MAP_FUNCTION(MAP_NAME, Find)(map, key);

  assert(i != -1); //FIXME: Desired behaviour?

  return map->values[i];
}

MAP_VAL MAP_FUNCTION(MAP_NAME, Take)(snet_map_t *map, MAP_KEY key)
{
  MAP_VAL result;
  int i = MAP_FUNCTION(MAP_NAME, Find)(map, key);
  assert(i != -1); //FIXME: Desired behaviour?

  result = map->values[i];

  map->used--;
  map->keys[i] = map->keys[map->used];
  map->values[i] = map->values[map->used];

  return result;
}

bool MAP_FUNCTION(MAP_NAME, Contains)(snet_map_t *map, MAP_KEY key)
{
  int i = MAP_FUNCTION(MAP_NAME, Find)(map, key);

  if (i == -1) {
    return false;
  }

  return true;
}



void MAP_FUNCTION(MAP_NAME, Rename)(snet_map_t *map, MAP_KEY oldKey, MAP_KEY newKey)
{
  int i = MAP_FUNCTION(MAP_NAME, Find)(map, oldKey);
  assert(i != -1); //FIXME: Desired behavior?
  map->keys[i] = newKey;
}

#ifdef MAP_CANARY
void MAP_FUNCTION(MAP_NAME, Serialise)(snet_map_t *map,
                                       void (*serialiseInts)(int, int*),
                                       void (*serialiseValues)(int, MAP_VAL*))
{
  serialiseInts(1, &map->used);
  serialiseInts(map->used, map->keys);
  serialiseValues(map->used, map->values);
}

void MAP_FUNCTION(MAP_NAME, Deserialise)(snet_map_t *map,
                                       void (*deserialiseInts)(int, int*),
                                       void (*deserialiseValues)(int, MAP_VAL*))
{
  deserialiseInts(1, &map->used);
  map->keys = SNetMemAlloc(map->used * sizeof(MAP_KEY));
  deserialiseInts(map->used, map->keys);
  map->values = SNetMemAlloc(map->used * sizeof(MAP_VAL));
  deserialiseValues(map->used, map->values);
}
#else /* MAP_CANARY */
void MAP_FUNCTION(MAP_NAME, Serialise)(snet_map_t *map,
                                       void (*serialiseInts)(int, int*),
                                       void (*serialiseKeys)(int, MAP_KEY*),
                                       void (*serialiseValues)(int, MAP_VAL*))
{
  serialiseInts(1, &map->used);
  serialiseKeys(map->used, map->keys);
  serialiseValues(map->used, map->values);
}

void MAP_FUNCTION(MAP_NAME, Deserialise)(snet_map_t *map,
                                       void (*deserialiseInts)(int, int*),
                                       void (*deserialiseKeys)(int, MAP_KEY*),
                                       void (*deserialiseValues)(int, MAP_VAL*))
{
  deserialiseInts(1, &map->used);
  map->keys = SNetMemAlloc(map->used * sizeof(MAP_KEY));
  deserialiseKeys(map->used, map->keys);
  map->values = SNetMemAlloc(map->used * sizeof(MAP_VAL));
  deserialiseValues(map->used, map->values);
}
#endif /* MAP_CANARY */

#ifdef MAP_CANARY
#undef MAP_KEY
#undef MAP_CANARY
#endif
#undef snet_map_t
#undef MAP_FUNCTION
#undef MAP
#undef CONCAT
