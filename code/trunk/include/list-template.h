#ifndef LIST_NAME_H
#error List requires LIST_NAME_H to be set. Function prefix will be
#error SNetLIST_NAME_H
#endif

#ifndef LIST_TYPE_NAME_H
#error List requires LIST_TYPE_NAME_H to be set.
#error Type will be snet_LIST_TYPE_NAME_H_list_t
#endif

#ifndef LIST_VAL_H
#error List requires LIST_VAL_H to be set. List content will be of type
#error LIST_VAL_H
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using ## results in behaviour like
 * "LIST(LIST_NAME)" expanding to "snet_LIST_NAME_list_t" instead of containing
 * the *value* of LIST_NAME.
 *
 * 2) These macros are partially duplicated in list-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define LIST(name)                      CONCAT(snet_, name, _list, _t)
#define LIST_FUNCTION(name, funName)    CONCAT(SNet, name, List, funName)
#define snet_list_t                     LIST(LIST_TYPE_NAME_H)

#define LIST_FOR_EACH(list, val) {\
  int snet_list_ctr;\
  for (snet_list_ctr = 0; snet_list_ctr < list->used; snet_list_ctr++) {\
    val = list->values[snet_list_ctr];

#define END_FOR                     } }

#define LIST_ENUMERATE(list, val, index) \
  for (index = 0; index < list->used; index++) {\
    val = list->values[index];

#define END_ENUMERATE               }

#define LIST_ZIP_EACH(list1, val1, list2, val2) {\
  int snet_list_ctr;\
  for (snet_list_ctr = 0;\
       snet_list_ctr < list1->used && snet_list_ctr < list2->used;\
       snet_list_ctr++) {\
    val1 = list1->values[snet_list_ctr];\
    val2 = list2->values[snet_list_ctr];

#define LIST_ZIP_ENUMERATE(list1, val1, list2, val2, index) {\
  for (index = 0; index < list1->used && index < list2->used; index++) {\
    val1 = list1->values[index];\
    val2 = list2->values[index];\

#define END_ZIP                     } }

typedef struct snet_list_t {
  int size, used;
  LIST_VAL_H *values;
} snet_list_t;

snet_list_t *LIST_FUNCTION(LIST_NAME_H, Create)(int size, ...);
snet_list_t *LIST_FUNCTION(LIST_NAME_H, Copy)(snet_list_t *list);
void LIST_FUNCTION(LIST_NAME_H, Destroy)(snet_list_t *list);

snet_list_t *LIST_FUNCTION(LIST_NAME_H, DeepCopy)(
        snet_list_t *list,
        LIST_VAL_H (*copyfun)(LIST_VAL_H));

int LIST_FUNCTION(LIST_NAME_H, Length)(snet_list_t *list);

void LIST_FUNCTION(LIST_NAME_H, Push)(snet_list_t *list, LIST_VAL_H val);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Pop)(snet_list_t *list);

bool LIST_FUNCTION(LIST_NAME_H, Contains)(snet_list_t *list, LIST_VAL_H val);

LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Get)(snet_list_t *list, int i);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Remove)(snet_list_t *list, int i);

#undef snet_list_t
#undef LIST_FUNCTION
#undef LIST
#undef CONCAT
