#include <assert.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/param.h>

#include "resdefs.h"

void res_loop(int listen)
{
  fd_set        rset, wset, rout, wout, *wptr;
  int           max_sock, num, sock, bit, io;
  intmap_t*     sock_map = res_map_create();
  intmap_t*     client_map = res_map_create();
  bitmap_t      bitmap = 0;
  int           wcnt = 0, loops = 0;
  const int     max_loops = 20;

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max_sock = listen;

  while (++loops <= max_loops) {
    bitmap_t rebalance = BITMAP_ZERO;

    rout = rset;
    wout = wset;
    wptr = ((wcnt > 0) ? &wout : NULL);
    num = select(max_sock + 1, &rout, wptr, NULL, NULL);
    if (num <= 0) {
      pexit("select");
    }

    if (FD_ISSET(listen, &rout)) {
      --num;
      if ((sock = res_accept_socket(listen, true)) == -1) {
        break;
      } else {
        for (bit = 0; bit <= MAX_BIT; ++bit) {
          if (NOT(bitmap, bit)) {
            break;
          }
        }
        if (bit > MAX_BIT) {
          res_warn("Insufficient room.\n");
          res_socket_send(sock, "{ full } \n", 10);
          res_socket_close(sock);
        } else {
          client_t *client = res_client_create(bit, sock);
          res_map_add(client_map, bit, client);
          res_map_add(sock_map, sock, client);
          FD_SET(sock, &rset);
          max_sock = MAX(max_sock, sock);
        }
      }
    }

    for (sock = 1 + listen; num > 0 && sock <= max_sock; ++sock) {
      if (FD_ISSET(sock, &rout) || FD_ISSET(sock, &wout)) {
        client_t* client = res_map_get(sock_map, sock);
        --num;
        if (FD_ISSET(sock, &rout)) {
          FD_CLR(sock, &rset);
          io = res_client_read(client);
          if (io >= 0 && client->rebalance) {
            client->rebalance = false;
            SET(rebalance, client->bit);
          }
        } else {
          FD_CLR(sock, &wset);
          --wcnt;
          assert(wcnt >= 0);
          io = res_client_write(client);
        }
        if (io == -1) {
          res_release_client(client);
          res_client_destroy(client);
          res_map_del(client_map, client->bit);
          res_map_del(sock_map, sock);
          rebalance = BITMAP_ALL;
          if (sock == max_sock) {
            while (--max_sock > listen &&
                   ! FD_ISSET(max_sock, &rset) &&
                   ! FD_ISSET(max_sock, &wset)) { }
          }
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
      res_rebalance(client_map);
      for (sock = 1 + listen; sock <= max_sock; ++sock) {
        if (FD_ISSET(sock, &rset)) {
          client_t* client = res_map_get(sock_map, sock);
          if (res_client_writing(client)) {
            FD_SET(sock, &wset);
            FD_CLR(sock, &rset);
            ++wcnt;
          }
        }
      }
    }
  }
  res_info("%s: Maximum number of loops reached (%d).\n",
           __func__, max_loops);

  res_map_apply(client_map, (void (*)(void *)) res_client_destroy);
  res_map_destroy(client_map);
  res_map_destroy(sock_map);
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

