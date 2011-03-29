#include <stdarg.h>
#include <string.h>

#include "memfun.h"

#ifndef NAME
#error Map requires a NAME value to be defined.
#endif

#ifndef VAL
#error Map requires a VAL value to be defined.
#endif

/* !! CAUTION !!
 * The seemingly useless indirections using CONCAT and MAP(name) is to ensure
 * the macro's get fully expanded properly. Removing them (for example defining
 * snet_map_t directly using CONCAT or FUNCTION/MAP directly using ## results
 * in behaviour like "MAP(NAME)" expanding to "snet_map_NAME_t" instead of
 * containing the *value* of NAME.
 */
#define CONCAT(prefix, s, suffix)  prefix ## s ## suffix
#define MAP(name)                  CONCAT(snet_map_, name, _t)
#define FUNCTION(name, funName)    CONCAT(SNetMap, name, funName)
#define snet_map_t                 MAP(NAME)
#define MAP_FOR_EACH(map)              \
{\
  int snet_map_ctr, snet_map_key;\
  VAL snet_map_val;\
  for (snet_map_ctr = 0; snet_map_ctr < map->used; snet_map_ctr++) {\
    snet_map_key = map->keys[snet_map_ctr];\
    snet_map_val = map->values[snet_map_ctr];

#define END_FOR                     } }
#define MAP_KEY                     snet_map_key
#define MAP_VAL                     snet_map_val

typedef struct {
  int size, used;
  VAL error;
  int *keys;
  VAL *values;
} snet_map_t;

static snet_map_t *FUNCTION(NAME, Create)(int size, VAL error, ...)
{
  int i;
  va_list args;
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = size;
  result->used = size;
  result->error = error;
  result->keys = SNetMemAlloc(size * sizeof(int));
  result->values = SNetMemAlloc(size * sizeof(VAL));

  va_start(args, error);
  for (i = 0; i < size; i++) {
    result->keys[i] = va_arg(args, int);
    result->values[i] = va_arg(args, VAL);
  }
  va_end(args);

  return result;
}

static snet_map_t *FUNCTION(NAME, Copy)(snet_map_t *map)
{
  snet_map_t *result = SNetMemAlloc(sizeof(snet_map_t));

  result->size = map->used;
  result->used = map->used;

  result->keys = SNetMemAlloc(map->used * sizeof(int));
  memcpy(result->keys, map->keys, map->used * sizeof(int));

  result->values = SNetMemAlloc(map->used * sizeof(VAL));
  memcpy(result->values, map->values, map->used * sizeof(VAL));

  return result;
}

static void FUNCTION(NAME, Destroy)(snet_map_t *map)
{
  SNetMemFree(map->keys);
  SNetMemFree(map->values);
  SNetMemFree(map);
}



static int FUNCTION(NAME, Find)(snet_map_t *map, int key)
{
  int i;
  for (i = 0; i < map->used; i++) {
    if (map->keys[i] == key) {
      return i;
    }
  }

  return -1;
}

static int FUNCTION(NAME, Size)(snet_map_t *map)
{
  return map->used;
}

static void FUNCTION(NAME, Set)(snet_map_t *map, int key, VAL val)
{
  int i = FUNCTION(NAME, Find)(map, key);

  if (i != -1) {
    map->values[i] = val;
  } else if (map->used < map->size) {
    map->keys[map->used] = key;
    map->values[map->used] = val;
    map->used++;
  } else {
    int *keys = SNetMemAlloc((map->size + 1) * sizeof(int));
    VAL *values = SNetMemAlloc((map->size + 1) * sizeof(VAL));

    memcpy(keys, map->keys, map->size * sizeof(int));
    memcpy(values, map->values, map->size * sizeof(VAL));

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

static VAL FUNCTION(NAME, Get)(snet_map_t *map, int key)
{
  int i = FUNCTION(NAME, Find)(map, key);

  if (i == -1) {
    return map->error;
  }

  return map->values[i];
}

static VAL FUNCTION(NAME, Take)(snet_map_t *map, int key)
{
  VAL result;
  int i = FUNCTION(NAME, Find)(map, key);
  if (i == -1) {
    return map->error;
  }

  result = map->values[i];

  map->used--;
  map->keys[i] = map->keys[map->used];
  map->values[i] = map->values[map->used];

  return result;
}

#undef snet_map_t
#undef FUNCTION
#undef MAP
#undef CONCAT
