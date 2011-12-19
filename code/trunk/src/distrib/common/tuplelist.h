#ifndef TUPLELIST_H
#define TUPLELIST_H

#include "bool.h"
#include "distribcommon.h"

#define LIST_NAME_H Tuple
#define LIST_TYPE_NAME_H tuple
#define LIST_VAL_H snet_tuple_t
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

#endif
