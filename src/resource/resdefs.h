#ifndef RESDEFS_H_INCLUDED
#define RESDEFS_H_INCLUDED

#include <stdbool.h>

#define RES_DEFAULT_LISTEN_PORT 56389

#define xnew(x)         (x *) xmalloc(sizeof(x))

#include "resproto.h"

#endif
