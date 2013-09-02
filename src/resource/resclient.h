#ifndef RESCLIENT_H_INCLUDED
#define RESCLIENT_H_INCLUDED

typedef struct client client_t;

struct client {
  stream_t      stream;
  unsigned long access;
  int           bit;
  int           local_workload;
  int           remote_workload;
  int           local_granted;
  int           local_accepted;
  int           local_revoked;
  int           local_planned;
  bool          rebalance;
};

#endif
