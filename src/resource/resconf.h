#ifndef RESCONF_H_INCLUDED
#define RESCONF_H_INCLUDED

struct res_server_conf {
  intmap_t*     slaves;
  const char*   listen_addr;
  int           listen_port;
  bool          debug;
  bool          verbose;
};

struct res_client_conf {
  const char*   server_addr;
  int           server_port;
  double        slowdown;
};

#endif
