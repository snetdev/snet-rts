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
  stream_t      stream;
  unsigned long systems;
  unsigned long access;
  enum server_state state;
  int           local;
  int           assigned;
  unsigned long assignmask;
};

#endif
