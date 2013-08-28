#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/param.h>

#include "resdefs.h"

typedef struct intmap intmap_t;
typedef struct buffer buffer_t;
typedef struct stream stream_t;
typedef struct client client_t;

struct intmap {
  void        **map;
  int           max;
};

struct buffer {
  char*         data;
  int           size;
  int           start;
  int           end;
};

struct stream {
  int           fd;
  buffer_t      read;
  buffer_t      write;
};

struct client {
  stream_t      stream;
};

intmap_t* res_map_create(void)
{
  intmap_t*     map = xnew(intmap_t);
  map->map = NULL;
  map->max = 0;
  return map;
}

void res_map_add(intmap_t* map, int key, void* val)
{
  assert(key >= 0);
  assert(val != NULL);
  if (key >= map->max) {
    int i, newmax = key + 10;
    map->map = xrealloc(map->map, newmax);
    for (i = map->max; i < newmax; ++i) {
      map->map[i] = NULL;
    }
    map->max = newmax;
  }
  assert(map->map[key] == NULL);
  map->map[key] = val;
}

void* res_map_get(intmap_t* map, int key)
{
  assert(key >= 0);
  assert(key < map->max);
  assert(map->map[key] != NULL);
  return map->map[key];
}

void res_map_del(intmap_t* map, int key)
{
  assert(key >= 0);
  assert(key < map->max);
  assert(map->map[key] != NULL);
  map->map[key] = NULL;
}

void res_map_destroy(intmap_t* map)
{
  xfree(map->map);
  xfree(map);
}

void res_buffer_init(buffer_t* buf)
{
  buf->data = NULL;
  buf->size = 0;
  buf->start = 0;
  buf->end = 0;
}

void res_buffer_done(buffer_t* buf)
{
  xdel(buf->data);
}

void res_stream_init(stream_t* stream, int fd)
{
  stream->fd = fd;
  res_buffer_init(&stream->read);
  res_buffer_init(&stream->write);
}

void res_stream_done(stream_t* stream)
{
  res_buffer_done(&stream->read);
  res_buffer_done(&stream->write);
  close(stream->fd);
}

client_t* res_client_create(int fd)
{
  client_t *client = xnew(client_t);
  res_stream_init(&client->stream, fd);
  return client;
}

void res_client_delete(client_t* client)
{
  res_stream_done(&client->stream);
  xdel(client);
}

void res_client_read(client_t* client)
{
}

void res_client_write(client_t* client)
{
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
      if ((sock = res_accept_socket(listen, true)) == -1) {
        break;
      }
      FD_SET(sock, &rset);
      max = MAX(max, sock);
      res_map_add(map, sock, res_client_create(sock));
    }

    for (i = 0; i <= max; ++i) {
      if (FD_ISSET(i, &rset)) {
        res_client_read(res_map_get(map, i));
      }
      else if (FD_ISSET(i, &wset)) {
        res_client_write(res_map_get(map, i));
      }
    }
  }

  for (i = 0; i < map->max; ++i) {
    if (map->map[i]) {
      res_client_delete(map->map[i]);
    }
  }
  res_map_destroy(map);
  close(listen);
}

