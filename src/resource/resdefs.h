#ifndef RESDEFS_H_INCLUDED
#define RESDEFS_H_INCLUDED

#ifndef __bool_true_false_are_defined
#include <stdbool.h>
#endif

#define RES_DEFAULT_LISTEN_PORT 56389

#define xnew(x)         (x *) xmalloc(sizeof(x))

typedef unsigned long bitmap_t;

#include "restypes.h"
#include "resstream.h"
#include "resclient.h"
#include "resproto.h"

#endif
