#ifndef MAP_NAME_H
#error Map requires MAP_NAME_H to be set. Function prefix will be SNetMAP_NAME_H
#endif

#ifndef MAP_TYPE_NAME_H
#error Map requires MAP_TYPE_NAME to be set.
#error Type will be snet_MAP_TYPE_NAME_map_t
#endif

#ifndef MAP_KEY_H
#define MAP_KEY_H int
#define MAP_CANARY_H
#endif

#ifndef MAP_VAL_H
#error Map requires MAP_VAL_H to be set. List content will be of type MAP_VAL_H
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and MAP(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_map_t directly using ## results in behaviour like
 * "MAP(MAP_NAME_H)" expanding to "snet_MAP_NAME_H_map_t" instead of containing
 * the *value* of MAP_NAME.
 *
 * 2) These macros are partially duplicated in map-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in set-template.c and set-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define MAP(name)                       CONCAT(snet_, name, _map, _t)
#define snet_map_t                      MAP(MAP_TYPE_NAME_H)
#define MAP_FUNCTION(name, funName)     CONCAT(SNet, name, Map, funName)

#define MAP_FOR_EACH(map, key, val) \
  for (int snet_map_ctr = 0; \
       (key = map->used \
              ? map->keys[snet_map_ctr] \
              : key), \
       (val = map->used \
              ? map->values[snet_map_ctr] \
              : val), \
       snet_map_ctr < map->used; \
       snet_map_ctr++)

typedef struct snet_map_t {
  int size, used;
  MAP_KEY_H *keys;
  MAP_VAL_H *values;
} snet_map_t;

snet_map_t *MAP_FUNCTION(MAP_NAME_H, Create)(int size, ...);
snet_map_t *MAP_FUNCTION(MAP_NAME_H, Copy)(snet_map_t *map);
snet_map_t *MAP_FUNCTION(MAP_NAME_H, ManualCopy)(
    snet_map_t *map,
    MAP_VAL_H (*copyFun)(MAP_VAL_H)
);
void MAP_FUNCTION(MAP_NAME_H, Destroy)(snet_map_t *map);

int MAP_FUNCTION(MAP_NAME_H, Size)(snet_map_t *map);

void MAP_FUNCTION(MAP_NAME_H, Set)(snet_map_t *map, MAP_KEY_H key, MAP_VAL_H val);
MAP_VAL_H MAP_FUNCTION(MAP_NAME_H, Get)(snet_map_t *map, MAP_KEY_H key);
MAP_VAL_H MAP_FUNCTION(MAP_NAME_H, Take)(snet_map_t *map, MAP_KEY_H key);
bool MAP_FUNCTION(MAP_NAME_H, Contains)(snet_map_t *map, MAP_KEY_H key);

void MAP_FUNCTION(MAP_NAME_H, Rename)(
    snet_map_t *map,
    MAP_KEY_H oldKey,
    MAP_KEY_H newKey
);

void MAP_FUNCTION(MAP_NAME_H, Serialise)(
    snet_map_t *map, void (*serialiseInts)(int, int*),
    #ifndef MAP_CANARY_H
    void (*serialiseKeys)(int, MAP_KEY_H*),
    #endif
    void (*serialiseValues)(int, MAP_VAL_H*)
);

void MAP_FUNCTION(MAP_NAME_H, Deserialise)(
    snet_map_t *map, void (*deserialiseInts)(int, int*),
    #ifndef MAP_CANARY_H
    void (*deserialiseKeys)(int, MAP_KEY_H*),
    #endif
    void (*deserialiseValues)(int, MAP_VAL_H*)
);

#ifdef MAP_CANARY_H
#undef MAP_KEY_H
#undef MAP_CANARY_H
#endif
#undef snet_map_t
#undef MAP_FUNCTION
#undef MAP
#undef CONCAT
