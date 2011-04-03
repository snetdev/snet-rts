#ifndef LIST_NAME_H
#error list requires a LIST_NAME_H value to be defined.
#endif

#ifndef LIST_VAL_H
#error list requires a LIST_VAL_H value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using CONCAT or LIST_FUNCTION/LIST directly using
 * ## results in behaviour like "LIST(LIST_NAME_H)" expanding to
 * "snet_LIST_NAME_H_list_t" instead of containing the *value* of LIST_NAME_H.
 *
 * 2) These macros are partially duplicated in list-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s, suffix)   prefix ## s ## suffix
#define CONCAT4(prefix, s1, s2, suffix) prefix ## s1 ## s2 ## suffix
#define LIST(name)                  CONCAT(snet_, name, _list_t)
#define LIST_FUNCTION(name, funName)     CONCAT4(SNet, name, List, funName)
#define snet_list_t                 LIST(LIST_NAME_H)

#define LIST_FOR_EACH(list, val) {\
  int snet_list_ctr;\
  for (snet_list_ctr = 0; snet_list_ctr < list->used; snet_list_ctr++) {\
    val = list->values[snet_list_ctr];

#define END_FOR                     } }

typedef struct snet_list_t {
  int size, used;
  LIST_VAL_H error;
  LIST_VAL_H *values;
} snet_list_t;

snet_list_t *LIST_FUNCTION(LIST_NAME_H, Create)(int size, LIST_VAL_H error, ...);
snet_list_t *LIST_FUNCTION(LIST_NAME_H, Copy)(snet_list_t *list);
void LIST_FUNCTION(LIST_NAME_H, Destroy)(snet_list_t *list);

int LIST_FUNCTION(LIST_NAME_H, Size)(snet_list_t *list);
void LIST_FUNCTION(LIST_NAME_H, Append)(snet_list_t *list, LIST_VAL_H val);
bool LIST_FUNCTION(LIST_NAME_H, Contains)(snet_list_t *list, LIST_VAL_H val);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Get)(snet_list_t *list, int i);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Remove)(snet_list_t *list, int i);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Pop)(snet_list_t *list);

#undef snet_list_t
#undef LIST_FUNCTION
#undef list
#undef CONCAT
