#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "resdefs.h"

typedef struct intmap intmap_t;
typedef int intmap_iter_t;

struct intmap {
  void        **map;
  int           max;
  int           num;
};

intmap_t* res_map_create(void)
{
  intmap_t* map = xnew(intmap_t);
  map->map = NULL;
  map->max = 0;
  map->num = 0;
  return map;
}

int res_map_max(intmap_t* map)
{
  return map->num - 1;
}

int res_map_count(intmap_t* map)
{
  int count = 0, i;
  for (i = 0; i < map->num; ++i) {
    if (map->map[i]) {
      ++count;
    }
  }
  return count;
}

void res_map_add(intmap_t* map, int key, void* val)
{
  assert(key >= 0);
  assert(val != NULL);
  if (key >= map->num) {
    if (key >= map->max) {
      int i, newmax = key + 10;
      map->map = xrealloc(map->map, newmax * sizeof(void *));
      for (i = map->max; i < newmax; ++i) {
        map->map[i] = NULL;
      }
      map->max = newmax;
    }
    map->num = key + 1;
  } else {
    assert(map->map[key] == NULL);
  }
  map->map[key] = val;
}

void* res_map_get(intmap_t* map, int key)
{
  assert(key >= 0);
  assert(key < map->num);
  assert(map->map[key] != NULL);
  return map->map[key];
}

void res_map_del(intmap_t* map, int key)
{
  assert(key >= 0);
  assert(key < map->num);
  assert(map->map[key] != NULL);
  map->map[key] = NULL;
  if (key + 1 == map->num) {
    while (--key >= 0 && map->map[key] == NULL) { }
    map->num = key + 1;
  }
}

void res_map_destroy(intmap_t* map)
{
  xfree(map->map);
  xfree(map);
}

void res_map_apply(intmap_t* map, void (*func)(void *))
{
  int i;
  for (i = 0; i < map->num; ++i) {
    if (map->map[i]) {
      (*func)(map->map[i]);
    }
  }
}

void res_map_iter_init(intmap_t* map, int* iter)
{
  *iter = -1;
}

void* res_map_iter_next(intmap_t* map, int* iter)
{
  assert(*iter >= -1);
  while (++*iter < map->num) {
    if (map->map[*iter]) {
      return map->map[*iter];
    }
  }
  return NULL;
}

