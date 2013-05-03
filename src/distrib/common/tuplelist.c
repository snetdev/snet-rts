#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "distribcommon.h"
#include "tuplelist.h"

#define LIST_NAME Tuple
#define LIST_TYPE_NAME tuple
#define LIST_VAL snet_tuple_t
#define LIST_CMP SNetTupleCompare
#include "list-template.c"
#undef LIST_CMP
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME
