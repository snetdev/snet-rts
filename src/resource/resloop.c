#include <assert.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/param.h>

#include "resdefs.h"


void res_rebalance_cores(intmap_t* map)
{
}

void res_rebalance_procs(intmap_t* map)
{
}

void res_rebalance_proportional(intmap_t* map)
{
}

void res_rebalance_minimal(intmap_t* map)
{
}

void res_rebalance(intmap_t* map)
{
  client_t* client;
  int count = 0;
  int load = 0;
  int nprocs = res_local_procs();
  int ncores = res_local_cores();
  intmap_iter_t iter;

  res_map_iter_init(map, &iter);
  while ((client = res_map_iter_next(map, &iter)) != NULL) {
    if (client->local_workload >= 1) {
      ++count;
      load += client->local_workload;
    }
  }

  if (load <= ncores) {
    res_rebalance_cores(map);
  }
  else if (load <= nprocs) {
    res_rebalance_procs(map);
  }
  else if (count < nprocs) {
    res_rebalance_proportional(map);
  }
  else {
    res_rebalance_minimal(map);
  }
}

void res_loop(int listen)
{
  fd_set        rset, wset, rout, wout;
  int           max, num, sock;
  intmap_t*     map = res_map_create();
  int           wcnt = 0, loops = 0;
  const int     max_loops = 10;
  
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max = listen;

  while (++loops <= max_loops) {
    bool rebalance = false;

    rout = rset;
    wout = wset;
    num = select(max + 1, &rout, wcnt > 0 ? &wout : NULL, NULL, NULL);
    if (num <= 0) {
      pexit("select");
    }

    if (FD_ISSET(listen, &rout)) {
      FD_CLR(listen, &rout);
      --num;
      if ((sock = res_accept_socket(listen, true)) == -1) {
        break;
      } else {
        FD_SET(sock, &rset);
        max = MAX(max, sock);
        res_map_add(map, sock, res_client_create(sock));
      }
    }

    for (sock = 1 + listen; num > 0 && sock <= max; ++sock) {
      if (FD_ISSET(sock, &rout) || FD_ISSET(sock, &wout)) {
        client_t* client = res_map_get(map, sock);
        int io;
        --num;
        if (FD_ISSET(sock, &rout)) {
          FD_CLR(sock, &rset);
          io = res_client_read(client);
          if (io >= 0 && client->rebalance) {
            client->rebalance = false;
            rebalance = true;
          }
        } else {
          FD_CLR(sock, &wset);
          --wcnt;
          assert(wcnt >= 0);
          io = res_client_write(client);
        }
        if (io == -1) {
          res_client_destroy(client);
          res_map_del(map, sock);
          rebalance = true;
        } else {
          if (res_client_writing(client)) {
            FD_SET(sock, &wset);
            ++wcnt;
          } else {
            FD_SET(sock, &rset);
          }
        }
      }
    }

    if (rebalance) {
      res_rebalance(map);
    }
  }
  res_info("%s: Maximum number of loops reached (%d).\n",
           __func__, max_loops);

  res_map_apply(map, (void (*)(void *)) res_client_destroy);
  res_map_destroy(map);
}

void res_service(int port)
{
  int listen = res_listen_socket(port, true);
  if (listen < 0) {
    exit(1);
  } else {
    res_loop(listen);
    res_socket_close(listen);
  }
}

