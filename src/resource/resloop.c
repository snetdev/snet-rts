#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "resdefs.h"

void res_loop(int port)
{
  fd_set rset, wset, rout, wout;
  int listen, max, n, s;
  
  listen = res_listen_socket(port, true);
  if (listen < 0) exit(1);

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max = listen;

  while (true) {
    rout = rset;
    wout = wset;
    n = select(max + 1, &rout, &wout, NULL, NULL);
    if (n <= 0) {
      pexit("select");
    }

    if (FD_ISSET(listen, &rout)) {
      if ((s = res_accept_socket(listen, true)) == -1) {
        break;
      }
      FD_SET(s, &rset);
    }
  }
}

