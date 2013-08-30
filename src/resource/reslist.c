#include <assert.h>
#include <stdlib.h>
#include "resdefs.h"

typedef struct intlist intlist_t;

struct intlist {
  int          *list;
  int           num;
  int           max;
};

void res_list_init(intlist_t* list)
{
  list->list = NULL;
  list->num = 0;
  list->max = 0;
}

intlist_t* res_list_create(void)
{
  intlist_t*     list = xnew(intlist_t);
  res_list_init(list);
  return list;
}

void res_list_reset(intlist_t* list)
{
  if (list->num > 0) {
    xfree(list->list);
    list->num = 0;
    list->max = 0;
  }
}

void res_list_move(intlist_t* dest, intlist_t* src)
{
  if (dest != src) {
    res_list_reset(dest);
    *dest = *src;
    src->list = NULL;
    src->num = 0;
    src->max = 0;
  }
}

void res_list_copy(intlist_t* dest, intlist_t* src)
{
  if (dest != src) {
    int i;
    if (dest->max < src->num) {
      dest->list = xrealloc(dest->list, src->num * sizeof(int));
      dest->num = dest->max = src->num;
    }
    for (i = 0; i < src->num; ++i) {
      dest->list[i] = src->list[i];
    }
  }
}

void res_list_set(intlist_t* list, int key, int val)
{
  assert(key >= 0);
  if (key >= list->num) {
    if (key >= list->max) {
      int i, newmax = 3 * (key + 8) >> 1;
      assert(newmax <= 10*1024);
      list->list = xrealloc(list->list, newmax * sizeof(int));
      for (i = list->max; i < newmax; ++i) {
        list->list[i] = -1;
      }
      list->max = newmax;
    }
    list->num = 1 + key;
  }
  list->list[key] = val;
}

int res_list_get(intlist_t* list, int key)
{
  assert(key >= 0);
  assert(key < list->num);
  return list->list[key];
}

void res_list_done(intlist_t* list)
{
  xfree(list->list);
  list->list = NULL;
}

void res_list_destroy(intlist_t* list)
{
  res_list_done(list);
  xfree(list);
}

int res_list_size(intlist_t* list)
{
  return list->num;
}

void res_list_append(intlist_t* list, int val)
{
  res_list_set(list, list->num, val);
}

