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

static jmp_buf res_parse_exception_context;

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

client_t* res_client_create(int bit, int fd)
{
  client_t *client = xnew(client_t);
  res_stream_init(&client->stream, fd);
  client->bit = 0;
  client->access = 0;
  client->access = 0;
  client->local_workload = 0;
  client->remote_workload = 0;
  client->rebalance = false;
  client->local_granted = 0;
  return client;
}

void res_client_destroy(client_t* client)
{
  res_stream_done(&client->stream);
  xdel(client);
}

void res_client_throw(void)
{
  longjmp(res_parse_exception_context, 1);
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
      char    name[10];
      int     len;
    } tokens[] = {
      { Left,     "{",        1 },
      { Right,    "}",        1 },
      { List,     "list",     4 },
      { Topology, "topology", 8 },
      { Access,   "access",   6 },
      { Local,    "local",    5 },
      { Remote,   "remote",   6 },
      { Accept,   "accept",   6 },
      { Return,   "return",   6 },
      { Systems,  "systems",  7 },
      { Hardware, "hardware", 8 },
      { Grant,    "grant",    5 },
      { Revoke,   "revoke",   6 },
    };
    const int num_tokens = sizeof(tokens) / sizeof(tokens[0]);
    int i, max = end - p;
    for (i = 0; i < num_tokens; ++i) {
      if (*p == tokens[i].name[0] &&
           (tokens[i].len == 1 ||
             (tokens[i].len < max &&
              !strncmp(p, tokens[i].name, tokens[i].len) &&
              !isalpha((unsigned char) p[tokens[i].len]))))
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
    res_info("Expected %s, got %s\n",
             res_token_string(expect), res_token_string(got));
    res_client_throw();
  }
}

void res_client_number(client_t* client, int* number)
{
  const int expect = Number;
  token_t got = res_client_token(client, number);
  if (got != expect) {
    res_info("Expected %s, got %s\n",
             res_token_string(expect), res_token_string(got));
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
  } else {
    res_stream_take(&client->stream, p - data);
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
    snprintf(str, size, "{ hardware %d host %s children 1 \n", id, host);
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
  int size = res_list_size(ints);
  int i;
  unsigned long access = 0;

  for (i = 0; i < size; i++) {
    int val = res_list_get(ints, i);
    if (val < 0 || val >= 8*sizeof(client->access)) {
      res_list_destroy(ints);
      res_client_throw();
    } else {
      access |= (1UL << val);
    }
  }
  res_list_destroy(ints);

  if (client->access != access) {
    client->access = access;
    client->rebalance = true;
  }
}

void res_client_command_local(client_t* client)
{
  int load = 0;
  res_client_number(client, &load);
  if (client->local_workload != load) {
    client->local_workload = load;
    client->rebalance = true;
  }
}

void res_client_command_remote(client_t* client)
{
  int load = 0;
  res_client_number(client, &load);
  client->remote_workload = load;
  if (client->remote_workload != load) {
    client->remote_workload = load;
    client->rebalance = true;
  }
}

void res_client_command_accept(client_t* client)
{
  intlist_t* ints = res_client_intlist(client);

  res_list_destroy(ints);
}

void res_client_command_return(client_t* client)
{
  intlist_t* ints = res_client_intlist(client);

  res_list_destroy(ints);
}

void res_client_command(client_t* client)
{
  token_t command = res_client_token(client, NULL);
  switch (command) {
    case List:     res_client_command_systems(client); break;
    case Topology: res_client_command_topology(client); break;
    case Access:   res_client_command_access(client); break;
    case Local:    res_client_command_local(client); break;
    case Remote:   res_client_command_remote(client); break;
    case Accept:   res_client_command_accept(client); break;
    case Return:   res_client_command_return(client); break;
    default:
      res_info("Unexpected token %s.\n", res_token_string(command));
      res_client_throw();
      break;
  }
}

void res_client_parse(client_t* client)
{
  res_client_expect(client, Left);
  res_client_command(client);
  res_client_expect(client, Right);
}

int res_client_process(client_t* client)
{
  if (setjmp(res_parse_exception_context)) {
    return -1;
  } else {
    res_client_parse(client);
    return 0;
  }
}

int res_client_complete(client_t* client)
{
  int   size = 0, nesting = 1;
  char *data = res_stream_incoming(&client->stream, &size);
  char *p, *end = data + size;
  enum { Complete = 1, Incomplete = 0, Failure = -1 };

  for (p = data; p < end && isspace((unsigned char) *p); ++p) {
    /* skip whitespace */
  }
  if (p < end) {
    if (*p != '{') {
      res_info("Expected left brace, got 0x%02x\n", (unsigned char) *p);
      return Failure;
    } else {
      char *preamble = p;
      while (++p < end) {
        if (*p == '{') ++nesting;
        if (*p == '}' && --nesting == 0) {
          break;
        }
      }
      if (preamble > data) {
        res_stream_take(&client->stream, preamble - data);
      }
    }
  }
  return nesting ? ((size < 10*1024) ? Incomplete : Failure) : Complete;
}

int res_client_write(client_t* client)
{
  return res_stream_write(&client->stream);
}

bool res_client_writing(client_t* client)
{
  return res_stream_writing(&client->stream);
}

int res_client_read(client_t* client)
{
  enum { Complete = 1, Incomplete = 0, Failure = -1 };

  if (res_stream_read(&client->stream) == Failure) {
    return Failure;
  } else {
    int complete;
    while ((complete = res_client_complete(client)) == Complete) {
      if (res_client_process(client) == Failure) {
        return Failure;
      }
    }
    if (complete == Failure) {
      return Failure;
    }
    else if (res_client_writing(client)) {
      return res_client_write(client);
    }
  }
  return 0;
}

