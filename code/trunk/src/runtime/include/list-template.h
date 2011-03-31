#ifndef LIST_NAME
#error list requires a LIST_NAME value to be defined.
#endif

#ifndef LIST_VAL
#error list requires a LIST_VAL value to be defined.
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using CONCAT or FUNCTION/LIST directly using
 * ## results in behaviour like "LIST(LIST_NAME)" expanding to
 * "snet_LIST_NAME_list_t" instead of containing the *value* of LIST_NAME.
 *
 * 2) These macros are partially duplicated in list-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s, suffix)   prefix ## s ## suffix
#define CONCAT4(prefix, s1, s2, suffix) prefix ## s1 ## s2 ## suffix
#define LIST(name)                  CONCAT(snet_, name, _list_t)
#define FUNCTION(name, funName)     CONCAT4(SNet, name, List, funName)
#define snet_list_t                 LIST(LIST_NAME)

#define LIST_FOR_EACH(list, val) {\
  int snet_list_ctr;\
  for (snet_list_ctr = 0; snet_list_ctr < list->used; snet_list_ctr++) {\
    val = list->values[snet_list_ctr];

#define END_FOR                     } }

typedef struct {
  int size, used;
  LIST_VAL error;
  int *keys;
  LIST_VAL *values;
} snet_list_t;

snet_list_t *FUNCTION(LIST_NAME, Create)(int size, LIST_VAL error, ...);
snet_list_t *FUNCTION(LIST_NAME, Copy)(snet_list_t *list);
void FUNCTION(LIST_NAME, Destroy)(snet_list_t *list);

int FUNCTION(LIST_NAME, Size)(snet_list_t *list);
void FUNCTION(LIST_NAME, Append)(snet_list_t *list, LIST_VAL val);
bool FUNCTION(LIST_NAME, Contains)(snet_list_t *list, LIST_VAL val);
LIST_VAL FUNCTION(LIST_NAME, Get)(snet_list_t *list, int i);
LIST_VAL FUNCTION(LIST_NAME, Remove)(snet_list_t *list, int i);
LIST_VAL FUNCTION(LIST_NAME, Pop)(snet_list_t *list);

#undef snet_list_t
#undef FUNCTION
#undef list
#undef CONCAT
