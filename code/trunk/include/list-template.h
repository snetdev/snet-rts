#ifndef LIST_NAME
#error List requires LIST_NAME to be set. Function prefix will be SNetLIST_NAME
#endif

#ifndef LIST_TYPE_NAME
#error List requires LIST_TYPE_NAME to be set.
#error Type will be snet_LIST_TYPE_NAME_list_t
#endif

#ifndef LIST_VAL
#error List requires LIST_VAL to be set. List content will be of type LIST_VAL
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using CONCAT or LIST_FUNCTION/LIST directly
 * using ## results in behaviour like "LIST(LIST_NAME)" expanding to
 * "snet_LIST_NAME_list_t" instead of containing the *value* of LIST_NAME.
 *
 * 2) These macros are partially duplicated in list-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define LIST(name)                      CONCAT(snet_, name, _list, _t)
#define LIST_FUNCTION(name, funName)    CONCAT(SNet, name, List, funName)
#define snet_list_t                     LIST(LIST_TYPE_NAME)

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
  LIST_VAL *values;
} snet_list_t;

snet_list_t *LIST_FUNCTION(LIST_NAME, Create)(int size, ...);
snet_list_t *LIST_FUNCTION(LIST_NAME, Copy)(snet_list_t *list);
snet_list_t *LIST_FUNCTION(LIST_NAME, DeepCopy)(snet_list_t *list, LIST_VAL (*copyfun)(LIST_VAL));
void LIST_FUNCTION(LIST_NAME, Destroy)(snet_list_t *list);

int LIST_FUNCTION(LIST_NAME, Length)(snet_list_t *list);
void LIST_FUNCTION(LIST_NAME, Append)(snet_list_t *list, LIST_VAL val);
LIST_VAL LIST_FUNCTION(LIST_NAME, Pop)(snet_list_t *list);

bool LIST_FUNCTION(LIST_NAME, Contains)(snet_list_t *list, LIST_VAL val);

LIST_VAL LIST_FUNCTION(LIST_NAME, Get)(snet_list_t *list, int i);
LIST_VAL LIST_FUNCTION(LIST_NAME, Remove)(snet_list_t *list, int i);

#undef snet_list_t
#undef LIST_FUNCTION
#undef LIST
#undef CONCAT
