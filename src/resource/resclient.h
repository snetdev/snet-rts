#ifndef RESCLIENT_H_INCLUDED
#define RESCLIENT_H_INCLUDED

#include "resstream.h"

struct remote {
  bitmap_t      assigning;
  bitmap_t      grantmap;
  bitmap_t      revoking;
};

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
  remote_t     *remotes;
  int           nremotes;
  int           remote_granted;
  int           remote_accepted;
  int           remote_revoked;
  bool          rebalance_local;
  bool          rebalance_remote;
  bool          shutdown;
};

#endif
