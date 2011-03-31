#ifndef MAP_NAME
#error Map requires a MAP_NAME value to be defined.
#endif

#ifndef MAP_VAL
#error Map requires a MAP_VAL value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using CONCAT or FUNCTION/MAP directly using ##
 * results in behaviour like "MAP(MAP_NAME)" expanding to "snet_map_MAP_NAME_t" instead
 * of containing the *value* of MAP_NAME.
 *
 * 2) These macros are partially duplicated in map-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s, suffix)  prefix ## s ## suffix
#define MAP(name)                  CONCAT(snet_, name, _map_t)
#define FUNCTION(name, funName)    CONCAT(SNetMap, name, funName)
#define snet_map_t                 MAP(MAP_NAME)

#define MAP_FOR_EACH(map, key, val) {\
  int snet_map_ctr;\
  for (snet_map_ctr = 0; snet_map_ctr < map->used; snet_map_ctr++) {\
    key = map->keys[snet_map_ctr];\
    val = map->values[snet_map_ctr];

#define END_FOR                     } }

typedef struct {
  int size, used;
  MAP_VAL error;
  int *keys;
  MAP_VAL *values;
} snet_map_t;

snet_map_t *FUNCTION(MAP_NAME, Create)(int size, MAP_VAL error, ...);
snet_map_t *FUNCTION(MAP_NAME, Copy)(snet_map_t *map);
void FUNCTION(MAP_NAME, Destroy)(snet_map_t *map);

int FUNCTION(MAP_NAME, Size)(snet_map_t *map);
void FUNCTION(MAP_NAME, Set)(snet_map_t *map, int key, MAP_VAL val);
MAP_VAL FUNCTION(MAP_NAME, Get)(snet_map_t *map, int key);
MAP_VAL FUNCTION(MAP_NAME, Take)(snet_map_t *map, int key);

#undef snet_map_t
#undef FUNCTION
#undef MAP
#undef CONCAT
