#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "resdefs.h"
#include "resclient.h"

static bool res_shutdown;

void res_loop(int listen)
{
  fd_set        rset, wset, rout, wout, *wptr;
  int           max_sock, num, sock, bit, io;
  intmap_t*     sock_map = res_map_create();
  intmap_t*     client_map = res_map_create();
  bitmap_t      bitmap = 0;
  int           wcnt = 0, loops = -1;
  const int     max_loops = 2000000000;

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max_sock = listen;

  while (++loops <= max_loops && res_shutdown == false) {
    bitmap_t rebalance = BITMAP_ZERO;

    rout = rset;
    wout = wset;
    wptr = ((wcnt > 0) ? &wout : NULL);
    num = select(max_sock + 1, &rout, wptr, NULL, NULL);
    if (num <= 0) {
      if (num == -1 && errno == EINTR) {
        continue;
      } else {
        res_pexit("select");
      }
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
          SET(bitmap, bit);
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
          res_map_del(client_map, client->bit);
          res_map_del(sock_map, sock);
          assert(HAS(bitmap, client->bit));
          CLR(bitmap, client->bit);
          if (client->shutdown == true) {
            res_shutdown = true;
          }
          res_client_destroy(client);
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
  res_map_apply(client_map, (void (*)(void *)) res_client_destroy);
  res_map_destroy(client_map);
  res_map_destroy(sock_map);

  res_info("%s: Terminating after %d loops.\n", __func__, loops);
}

char* res_slurp(void* vp)
{
  FILE* fp = (FILE *) vp;
  size_t size = MAX(20*1024, 3*PATH_MAX);
  size_t len = 0;
  char *str = xmalloc(size + 1);
  size_t n;
  str[0] = '\0';
  while ((n = fread(str + len, 1, size - len, fp)) >= 1) {
    len += n;
    str[len] = '\0';
    if (len < size) {
      if (ferror(fp)) {
        res_warn("Error while reading from slave: %s\n", strerror(errno));
        break;
      }
      if (feof(fp)) {
        break;
      }
    }
    if (len + PATH_MAX >= size) {
      size = 3 * size / 2;
      str = xrealloc(str, size + 1);
    }
    if (ferror(fp)) {
      res_warn("Error while reading from slave: %s\n", strerror(errno));
    }
  }
  return str;
}

void res_parse_slave(int sysid, char* output)
{
  char *start;
  if ((start = strstr(output, "{ system ")) != NULL) {
    int count = 1;
    char* str = start;
    while (*++str) {
      if (*str == '{') {
        ++count;
      }
      if (*str == '}') {
        if (--count == 0) {
          break;
        }
      }
    }
    if (count == 0) {
      if (res_parse_topology(sysid, start) == false) {
        res_error("[%s]: Parse error for output of slave %d\n", __func__, sysid);
      }
    } else {
      res_error("[%s]: Incomplete output of slave %d\n", __func__, sysid);
    }
  }
}

void res_start_slave(int sysid, char* command)
{
  FILE* fp = popen(command, "r");
  if (!fp) {
    res_warn("Failed to start slave %d: %s.\n", sysid, strerror(errno));
  } else {
    char* output = res_slurp(fp);
    int wait;
    if ((wait = pclose(fp)) != 0) {
      if (wait == -1) {
        res_warn("Execution of slave %d failed: %s.\n", sysid, strerror(errno));
      }
      else if (WIFEXITED(wait)) {
        res_warn("Slave %d exited with code %d.\n", sysid, WEXITSTATUS(wait));
      }
      else if (WIFSIGNALED(wait)) {
        res_warn("Slave %d died with signal %d.\n", sysid, WTERMSIG(wait));
      }
      else {
        res_warn("Slave %d died magically: %d.\n", sysid, wait);
      }
    }
    res_parse_slave(sysid, output);
    xfree(output);
  }
}

void res_start_slaves(intmap_t* slaves)
{
  if (slaves) {
    intmap_iter_t iter = -1;
    char* str;
    res_map_iter_init(slaves, &iter);
    while ((str = (char *) res_map_iter_next(slaves, &iter)) != NULL) {
      res_start_slave(iter, str);
    }
    res_map_apply(slaves, xfree);
    res_map_destroy(slaves);
  }
}

void res_catch_sigint(int s)
{
  res_debug("%s: Caught SIGINT.\n", res_get_program_name());
  res_shutdown = true;
}

void res_catch_signals(void)
{
  struct sigaction act;

  memset(&act, 0, sizeof act);
  act.sa_sigaction = NULL;
  act.sa_restorer = NULL;
  act.sa_handler = res_catch_sigint;

  sigaction(SIGINT, &act, NULL);
}

void res_service(const char* listen_addr, int listen_port, intmap_t* slaves)
{
  int listen = res_listen_socket(listen_addr, listen_port, true);
  if (listen < 0) {
    exit(1);
  } else {
    res_start_slaves(slaves);
    res_catch_signals();
    res_loop(listen);
    res_socket_close(listen);
  }
}

