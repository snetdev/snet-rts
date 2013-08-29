#include <assert.h>
#include <stdarg.h>
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
typedef enum token token_t;

enum token {
  Notoken,
  Left,
  Right,
  Number,
  List,
  Topology,
  Access,
  Local,
  Remote,
  Accept,
  Return,
  Systems,
  Hardware,
  Grant,
  Revoke,
};

static jmp_buf res_client_exception_context;

struct client {
  stream_t      stream;
  int           localwl;
  int           remotewl;
  bool          rebalance;
};

const char *res_token_string(token_t token)
{
  return
#define NAME(x) #x
#define TOK(k) token == k ? NAME(k) :
  TOK(Notoken)
  TOK(Left)
  TOK(Right)
  TOK(Number)
  TOK(List)
  TOK(Topology)
  TOK(Access)
  TOK(Local)
  TOK(Remote)
  TOK(Accept)
  TOK(Return)
  TOK(Systems)
  TOK(Hardware)
  TOK(Grant)
  TOK(Revoke)
  "Unknown";
}

client_t* res_client_create(int fd)
{
  client_t *client = xnew(client_t);
  res_stream_init(&client->stream, fd);
  client->localwl = 0;
  client->remotewl = 0;
  client->rebalance = false;
  return client;
}

void res_client_destroy(client_t* client)
{
  res_stream_done(&client->stream);
  xdel(client);
}

void res_client_throw(void)
{
  longjmp(res_client_exception_context, 1);
}

token_t res_client_token(client_t* client, int* number)
{
  token_t token = Notoken;
  int length = 0;
  char* data = res_stream_incoming(&client->stream, &length);
  char* end = data + length;
  char* p;

  for (p = data; p < end && isspace((unsigned char) *p); ++p) {
    /* skip whitespace */
  }

  /* Impossible: a right brace must be present. */
  if (p >= end) {
    res_client_throw();
  }

  /* Check for numbers. */
  if (isdigit((unsigned char) *p)) {
    int n = *p - '0';
    while (++p < end && isdigit((unsigned char) *p)) {
      n = 10 * n + (*p - '0');
    }
    token = Number;
    if (number != NULL) {
      *number = n;
    }
  }
  else {
    /* Check for a list of textual tokens. */
    static const struct {
      token_t token;
      char name[10];
      int len;
    } tokens[] = {
      { Left, "{", 1 },
      { Right, "}", 1 },
      { List, "list", 4 },
      { Topology, "topology", 8 },
      { Access, "access", 6 },
      { Local, "local", 5 },
      { Remote, "remote", 6 },
      { Accept, "accept", 6 },
      { Return, "return", 6 },
      { Systems, "systems", 7 },
      { Hardware, "hardware", 8 },
      { Grant, "grant", 5 },
      { Revoke, "revoke", 6 },
    };
    const int num_tokens = sizeof(tokens) / sizeof(tokens[0]);
    int i, max = end - p;
    for (i = 0; i < num_tokens; ++i) {
      if (tokens[i].len <= max &&
          !strncmp(p, tokens[i].name, tokens[i].len))
      {
        token = tokens[i].token;
        p += tokens[i].len;
        break;
      }
    }
  }

  if (token == Notoken) {
    res_info("%s: Unexpectedly got %s\n", __func__, res_token_string(token));
    res_client_throw();
  }

  res_stream_take(&client->stream, p - data);

  return token;
}

void res_client_expect(client_t* client, token_t expect)
{
  token_t got = res_client_token(client, NULL);
  if (got != expect) {
    res_info("Expected %s, got %s\n", res_token_string(expect), res_token_string(got));
    res_client_throw();
  }
}

void res_client_number(client_t* client, int* number)
{
  const int expect = Number;
  token_t got = res_client_token(client, number);
  if (got != expect) {
    res_info("Expected %s, got %s\n", expect, got);
    res_client_throw();
  }
}

void res_client_reply(client_t* client, const char* fmt, ...)
{
  int size = 0;
  char* data = res_stream_outgoing(&client->stream, &size);
  if (size < 512) {
    res_stream_reserve(&client->stream, 1024);
    data = res_stream_outgoing(&client->stream, &size);
  }
  assert(size >= 512);
  while (true) {
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(data, size, fmt, ap);
    va_end(ap);
    if (n >= 0 && n < size) {
      res_stream_appended(&client->stream, n);
      break;
    } else {
      size = (n >= 0) ? (n + 1) : (2 * size);
      res_stream_reserve(&client->stream, size);
      data = res_stream_outgoing(&client->stream, &size);
    }
  }
}

intlist_t* res_client_intlist(client_t* client)
{
  intlist_t* list = res_list_create();
  int   length = 0;
  char* data = res_stream_incoming(&client->stream, &length);
  char* end = data + length;
  char* p;
  bool  found = true;

  while (found) {
    found = false;

    for (p = data; p < end && isspace((unsigned char) *p); ++p) {
      /* skip whitespace */
    }

    /* Check for numbers. */
    if (p < end && isdigit((unsigned char) *p)) {
      int n = *p - '0';
      while (++p < end && isdigit((unsigned char) *p)) {
        n = 10 * n + (*p - '0');
      }
      res_list_append(list, n);
    }
  }
  if (res_list_size(list) == 0) {
    res_list_destroy(list);
    res_client_throw();
  }
  return list;
}

void res_client_command_systems(client_t* client)
{
  // Currently support only the local system
  res_client_reply(client, "{ systems 0 }\n");
}

void res_client_command_topology(client_t* client)
{
  int id = 0;
  res_client_number(client, &id);
  if (id) {
    res_info("Invalid topology id.\n");
    res_client_throw();
  } else {
    int len, size = 10*1024;
    char host[100];
    char *str = xmalloc(size);
    str[0] = '\0';
    gethostname(host, sizeof host);
    host[sizeof host - 1] = '\0';
    snprintf(str, size, "{ hardware %d host %s \n", id, host);
    len = strlen(str);
    str = res_topo_string(NULL, str, len, &size);
    len += strlen(str + len);
    if (len + 10 > size) {
      size = len + 10;
      str = xrealloc(str, size);
    }
    snprintf(str + len, size - len, "} \n");
    res_client_reply(client, str);
    xfree(str);
  }
}

void res_client_command_access(client_t* client)
{
  intlist_t* ints = res_client_intlist(client);
  // Needed when we are going to support remote systems
  res_list_destroy(ints);
}

void res_client_command_local(client_t* client)
{
  int load = 0;
  res_client_number(client, &load);
  if (client->localwl != load) {
    client->localwl = load;
    client->rebalance = true;
  }
}

void res_client_command_remote(client_t* client)
{
  int load = 0;
  res_client_number(client, &load);
  client->remotewl = load;
  if (client->remotewl != load) {
    client->remotewl = load;
    client->rebalance = true;
  }
}

void res_client_command_accept(client_t* client)
{
  // intlist_t* ints = res_client_intlist(client);
}

void res_client_command_return(client_t* client)
{
  // intlist_t* ints = res_client_intlist(client);
}

void res_client_command(client_t* client)
{
  token_t command = res_client_token(client, NULL);
  switch (command) {
    case List: res_client_command_systems(client); break;
    case Topology: res_client_command_topology(client); break;
    case Access: res_client_command_access(client); break;
    case Local: res_client_command_local(client); break;
    case Remote: res_client_command_remote(client); break;
    case Accept: res_client_command_accept(client); break;
    case Return: res_client_command_return(client); break;
    default:
      res_info("Unexpected token %s.\n", res_token_string(command));
      res_client_throw();
      break;
  }
}

int res_client_process(client_t* client)
{
  if (setjmp(res_client_exception_context)) {
    return -1;
  } else {
    res_client_expect(client, Left);
    res_client_command(client);
    res_client_expect(client, Right);
    return 0;
  }
}

int res_client_complete(client_t* client)
{
  int size = 0;
  char* data = res_stream_incoming(&client->stream, &size);
  int count = 0;
  char* end = data + size;
  char* p;

  // res_stream_take(&client->stream, p - data);
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
  int           listen, max, num, sock;
  intmap_t*     map = res_map_create();
  int           wcnt = 0, loops = 20;
  
  listen = res_listen_socket(port, true);
  if (listen < 0) exit(1);

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(listen, &rset);
  max = listen;

  while (--loops >= 0) {
    rout = rset;
    wout = wset;
    assert(wcnt >= 0);
    num = select(max + 1, &rout, wcnt > 0 ? &wout : NULL, NULL, NULL);
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

    for (sock = listen; num > 0 && sock <= max; ++sock) {
      if (FD_ISSET(sock, &rout) || FD_ISSET(sock, &wout)) {
        client_t* client = res_map_get(map, sock);
        int io;
        --num;
        if (FD_ISSET(sock, &rout)) {
          io = res_client_read(client);
          FD_CLR(sock, &rset);
          if (io >= 0 && res_client_writing(client)) {
            io = res_client_write(client);
          }
        } else {
          io = res_client_write(client);
          FD_CLR(sock, &wset);
          --wcnt;
        }
        if (io == -1) {
          res_client_destroy(client);
          res_map_del(map, sock);
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
  }
  res_info("%s: Maximum number of loops reached.\n", __func__);

  res_map_iter_new(map, &sock);
  {
    client_t* client;
    while ((client = res_map_iter_next(map, &sock)) != NULL) {
      res_client_destroy(client);
    }
  }
  res_map_destroy(map);
  close(listen);
}

