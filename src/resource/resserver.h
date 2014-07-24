#ifndef RESSERVER_H_INCLUDED
#define RESSERVER_H_INCLUDED

#include "resstream.h"

enum server_state {
  ServerInit,
  ServerListSent,
  ServerListRcvd,
  ServerTopoSent,
  ServerTopoRcvd,
};

struct server {
  /* Bi-directional buffered network connection. */
  stream_t      stream;

  /* Bitmask of systems which the server knows about. */
  bitmap_t      systems;

  /* Bitmask of systems which can be accessed by us. */
  bitmap_t      access;

  /* Progress tracker in client/server-protocol. */
  enum server_state state;

  /* Current local workload. */
  int           local;

  /* Number of processors which we can use. */
  int           granted;

  /* Number of processors which we should give back. */
  int           revoked;

  /* Bitmask of usable processors. */
  bitmap_t      grantmask;

  /* Bitmask of assigned processors. */
  bitmap_t      assignmask;

  /* Bitmask of revoked processors. */
  bitmap_t      revokemask;
};

#endif
