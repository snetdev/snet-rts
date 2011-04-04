#ifndef MAP_NAME_H
#error Map requires a MAP_NAME_H value to be defined.
#endif

#ifndef MAP_VAL_H
#error Map requires a MAP_VAL_H value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using CONCAT or MAP_FUNCTION/MAP directly using ##
 * results in behaviour like "MAP(MAP_NAME_H)" expanding to "snet_map_MAP_NAME_H_t" instead
 * of containing the *value* of MAP_NAME_H.
 *
 * 2) These macros are partially duplicated in map-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s, suffix)  prefix ## s ## suffix
#define CONCAT4(prefix, s1, s2, suffix) prefix ## s1 ## s2 ## suffix
#define MAP(name)                  CONCAT(snet_, name, _map_t)
#define MAP_FUNCTION(name, funName)    CONCAT4(SNet, name, Map, funName)
#define snet_map_t                 MAP(MAP_NAME_H)

#define MAP_FOR_EACH(map, key, val) {\
  int snet_map_ctr;\
  for (snet_map_ctr = 0; snet_map_ctr < map->used; snet_map_ctr++) {\
    key = map->keys[snet_map_ctr];\
    val = map->values[snet_map_ctr];

#define END_FOR                     } }

typedef struct snet_map_t {
  int size, used;
  MAP_VAL_H error;
  int *keys;
  MAP_VAL_H *values;
} snet_map_t;

snet_map_t *MAP_FUNCTION(MAP_NAME_H, Create)(int size, MAP_VAL_H error, ...);
snet_map_t *MAP_FUNCTION(MAP_NAME_H, Copy)(snet_map_t *map);
void MAP_FUNCTION(MAP_NAME_H, Destroy)(snet_map_t *map);

int MAP_FUNCTION(MAP_NAME_H, Size)(snet_map_t *map);
void MAP_FUNCTION(MAP_NAME_H, Set)(snet_map_t *map, int key, MAP_VAL_H val);
MAP_VAL_H MAP_FUNCTION(MAP_NAME_H, Get)(snet_map_t *map, int key);
MAP_VAL_H MAP_FUNCTION(MAP_NAME_H, Take)(snet_map_t *map, int key);
void MAP_FUNCTION(MAP_NAME_H, Rename)(snet_map_t *map, int oldKey, int newKey);

#undef snet_map_t
#undef MAP_FUNCTION
#undef MAP
#undef CONCAT
