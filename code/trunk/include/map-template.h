#ifndef MAP_NAME
#error Map requires MAP_NAME to be set. Function prefix will be SNetLIST_NAME
#endif

#ifndef MAP_TYPE_NAME
#error Map requires MAP_TYPE_NAME to be set.
#error Type will be snet_MAP_TYPE_NAME_map_t
#endif

#ifndef MAP_VAL
#error Map requires MAP_VAL to be set. List content will be of type MAP_VAL
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using CONCAT or MAP_FUNCTION/MAP directly
 * using ## results in behaviour like "MAP(MAP_NAME_H)" expanding to
 * "snet_MAP_NAME_map_t" instead of containing the *value* of MAP_NAME.
 *
 * 2) These macros are partially duplicated in map-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define MAP(name)                       CONCAT(snet_, name, _map, _t)
#define MAP_FUNCTION(name, funName)     CONCAT(SNet, name, Map, funName)
#define snet_map_t                      MAP(MAP_TYPE_NAME)

#define MAP_FOR_EACH(map, key, val) {\
  int snet_map_ctr;\
  for (snet_map_ctr = 0; snet_map_ctr < map->used; snet_map_ctr++) {\
    key = map->keys[snet_map_ctr];\
    val = map->values[snet_map_ctr];

#define END_FOR                     } }

typedef struct snet_map_t {
  int size, used;
  int *keys;
  MAP_VAL *values;
} snet_map_t;

snet_map_t *MAP_FUNCTION(MAP_NAME, Create)(int size, ...);
snet_map_t *MAP_FUNCTION(MAP_NAME, Copy)(snet_map_t *map);
snet_map_t *MAP_FUNCTION(MAP_NAME, DeepCopy)(snet_map_t *map, MAP_VAL (*copyfun)(MAP_VAL));
void MAP_FUNCTION(MAP_NAME, Destroy)(snet_map_t *map);

int MAP_FUNCTION(MAP_NAME, Size)(snet_map_t *map);

void MAP_FUNCTION(MAP_NAME, Set)(snet_map_t *map, int key, MAP_VAL val);
bool MAP_FUNCTION(MAP_NAME, Contains)(snet_map_t *map, int key);
MAP_VAL MAP_FUNCTION(MAP_NAME, Get)(snet_map_t *map, int key);
MAP_VAL MAP_FUNCTION(MAP_NAME, Take)(snet_map_t *map, int key);
void MAP_FUNCTION(MAP_NAME, Rename)(snet_map_t *map, int oldKey, int newKey);

#undef snet_map_t
#undef MAP_FUNCTION
#undef MAP
#undef CONCAT
