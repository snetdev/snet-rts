#ifndef RESCLIENT_H_INCLUDED
#define RESCLIENT_H_INCLUDED

#include "resstream.h"

struct client {
  stream_t      stream;
  unsigned long access;
  int           bit;
  int           local_workload;
  int           remote_workload;
  int           local_granted;
  int           local_accepted;
  int           local_revoked;
  bitmap_t      local_assigning;
  bitmap_t      local_grantmap;
  bitmap_t      local_revoking;
  bool          rebalance_local;
  bool          rebalance_remote;
  bool          shutdown;
};

#endif
