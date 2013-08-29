#include <assert.h>
#include <stdlib.h>
#include "resdefs.h"

typedef struct intlist intlist_t;

struct intlist {
  int          *list;
  int           num;
  int           max;
};

intlist_t* res_list_create(void)
{
  intlist_t*     list = xnew(intlist_t);
  list->list = NULL;
  list->num = 0;
  list->max = 0;
  return list;
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

void res_list_destroy(intlist_t* list)
{
  xfree(list->list);
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

