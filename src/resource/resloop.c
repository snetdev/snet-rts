#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/param.h>
#include <setjmp.h>
#include <ctype.h>

#include "resdefs.h"

typedef struct client client_t;

static jmp_buf res_client_exception_context;

struct client {
  stream_t      stream;
};

client_t* res_client_create(int fd)
{
  client_t *client = xnew(client_t);
  res_stream_init(&client->stream, fd);
  return client;
}

void res_client_destroy(client_t* client)
{
  res_stream_done(&client->stream);
  xdel(client);
}

int res_client_catch(void)
{
  return setjmp(res_client_exception_context);
}

void res_client_throw(void)
{
  longjmp(res_client_exception_context, 1);
}

char res_client_char(client_t* client)
{
  int size = 0;
  char* data = res_stream_incoming(&client->stream, &size);
  char* end = data + size;
  char* p;
  for (p = data; p < end; ++p) {
    if (!isspace((unsigned char) *p)) {
      res_stream_take(&client->stream, p - data);
      return *p;
    }
  }
  res_client_throw();
  return '\0';
}

void res_client_consume_char(client_t* client, char sym)
{
  char ch = res_client_char(client);
  if (ch != sym) {
    res_client_throw();
  } else {
    res_stream_take(&client->stream, 1);
  }
}

void res_client_command(client_t* client)
{

}

int res_client_process(client_t* client)
{
  if (res_client_catch()) {
    return -1;
  } else {
    char *str;
    res_client_consume_char(client, '{');
    res_client_command(client);
    res_client_consume_char(client, '}');
  }
  return 0;
}

int res_client_complete(client_t* client)
{
  int size = 0;
  char* data = res_stream_incoming(&client->stream, &size);
  int count = 0;
  char* end = data + size;
  char* p;
  for (p = data; p < end; ++p) {
    if (*p == '{') ++count;
    if (*p == '}') {
      if (--count <= 0) {
        return (count < 0) ? -1 : (p + 1 - data);
      }
    }
  }
  return (size > 10*1024) ? -1 : 0;
}

int res_client_read(client_t* client)
{
  if (res_stream_read(&client->stream) == -1) {
    return -1;
  } else {
    int complete = res_client_complete(client);
    if (complete <= 0) {
      return complete;
    } else {
      int parse = res_client_process(client);
      if (parse == -1) {
        return parse;
      }
    }
  }
  return 0;
}

int res_client_write(client_t* client)
{
  if (res_stream_write(&client->stream) == -1) {
    return -1;
  }
  return 0;
}

bool res_client_writing(client_t* client)
{
  return res_stream_writing(&client->stream);
}

void res_loop(int port)
{
  fd_set        rset, wset, rout, wout;
  int           listen, max, num, sock, i;
  intmap_t*     map = res_map_create();
  
  listen = res_listen_socket(port, true);
  if (listen < 0) exit(1);

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max = listen;

  while (true) {
    rout = rset;
    wout = wset;
    num = select(max + 1, &rout, &wout, NULL, NULL);
    if (num <= 0) {
      pexit("select");
    }

    if (FD_ISSET(listen, &rout)) {
      --num;
      if ((sock = res_accept_socket(listen, true)) == -1) {
        break;
      }
      FD_SET(sock, &rset);
      max = MAX(max, sock);
      res_map_add(map, sock, res_client_create(sock));
    }

    for (i = 0; num > 0 && i <= max; ++i) {
      if (FD_ISSET(i, &rout) || FD_ISSET(i, &wout)) {
        client_t* client = res_map_get(map, i);
        int io;
        --num;
        if (FD_ISSET(i, &rout)) {
          io = res_client_read(client);
          FD_CLR(i, &rset);
        } else {
          io = res_client_write(client);
          FD_CLR(i, &wset);
        }
        if (io == -1) {
          res_client_destroy(client);
        } else {
          if (res_client_writing(client)) {
            FD_SET(i, &wset);
          } else {
            FD_SET(i, &rset);
          }
        }
      }
    }
  }

  res_map_iter_new(map, &i);
  {
    client_t* client;
    while ((client = res_map_iter_next(map, &i)) != NULL) {
      res_client_destroy(client);
    }
  }
  res_map_destroy(map);
  close(listen);
}

