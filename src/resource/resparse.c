#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/param.h>
#include <ctype.h>

#include "resdefs.h"
#include "resparse.h"
#include "resource.h"

typedef enum token token_t;

jmp_buf res_parse_exception_context;

const char *res_token_string(int token)
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
  TOK(Resources)
  TOK(Access)
  TOK(Local)
  TOK(Remote)
  TOK(Accept)
  TOK(Return)
  TOK(Systems)
  TOK(Hardware)
  TOK(Grant)
  TOK(Revoke)
  TOK(Quit)
  TOK(Help)
  TOK(State)
  TOK(Children)
  TOK(Logical)
  TOK(Physical)
  TOK(Numa)
  TOK(Hostname)
  TOK(String)
  TOK(System)
  TOK(Machine)
  TOK(Node)
  TOK(Socket)
  TOK(Cache)
  TOK(Core)
  TOK(Proc)
  TOK(Other)
  "Unknown";
#undef NAME
#undef TOK
}

void res_parse_throw(void)
{
  longjmp(res_parse_exception_context, 1);
}

token_t res_parse_string(const char* data, int* length, void* result)
{
  token_t token = Notoken;
  const char* end = data + *length;
  const char* p;

  for (p = data; p < end && isspace((unsigned char) *p); ++p) {
    /* skip whitespace */
  }

  /* Impossible: a right brace must be present. */
  if (p >= end) {
    res_parse_throw();
  }

  /* Check for numbers. */
  if (isdigit((unsigned char) *p)) {
    int n = *p - '0';
    while (++p < end && isdigit((unsigned char) *p)) {
      n = 10 * n + (*p - '0');
    }
    if ( ! isalpha((unsigned char) *p)) {
      token = Number;
      if (result != NULL) {
        *(int *)result = n;
      }
    }
  }
  else {
    /* Check for a list of textual tokens. */
    static const struct {
      token_t token;
      char    name[10];
      int     len;
    } tokens[] = {
      { Left,      "{",         1 },
      { Right,     "}",         1 },
      { List,      "list",      4 },
      { Topology,  "topology",  8 },
      { Resources, "resources", 9 },
      { Access,    "access",    6 },
      { Local,     "local",     5 },
      { Remote,    "remote",    6 },
      { Accept,    "accept",    6 },
      { Return,    "return",    6 },
      { Systems,   "systems",   7 },
      { System,    "system",    6 },
      { Hardware,  "hardware",  8 },
      { Grant,     "grant",     5 },
      { Revoke,    "revoke",    6 },
      { Quit,      "quit",      4 },
      { Help,      "help",      4 },
      { State,     "state",     5 },
      { Children,  "children",  8 },
      { Core,      "core",      4 },
      { Logical,   "logical",   7 },
      { Physical,  "physical",  8 },
      { Cache,     "cache",     5 },
      { Proc,      "proc",      4 },
      { Numa,      "numa",      4 },
      { Hostname,  "hostname",  8 },
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
    res_info("%s: Unexpectedly got %s at '%.10s'.\n",
             __func__, res_token_string(token), p);
    res_parse_throw();
  }

  *length = p - data;

  return token;
}

token_t res_parse_token(stream_t* stream, void* result)
{
  int length = 0;
  char* data = res_stream_incoming(stream, &length);
  int taken = length;
  token_t token = res_parse_string(data, &taken, result);

  assert(taken >= 0 && taken <= length);

  res_stream_take(stream, taken);

  return token;
}

int res_parse_complete(stream_t* stream)
{
  int   size = 0, nesting = 1;
  char *data = res_stream_incoming(stream, &size);
  char *p, *end = data + size;
  enum { Complete = 1, Incomplete = 0, Failure = -1 };

  for (p = data; p < end && isspace((unsigned char) *p); ++p) {
    /* skip whitespace */
  }
  if (p < end) {
    if (*p != '{') {
      res_info("%s: Expected left brace, got 0x%02x\n",
               __func__, (unsigned char) *p);
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
        res_stream_take(stream, preamble - data);
      }
    }
  }
  return nesting ? ((size < 10*1024) ? Incomplete : Failure) : Complete;
}

intlist_t* res_parse_intlist(stream_t* stream)
{
  intlist_t* list = res_list_create();
  int   length = 0;
  char* data = res_stream_incoming(stream, &length);
  char* end = data + length;
  char* p = data;
  bool  found = true;

  while (found) {
    found = false;

    while (p < end && isspace((unsigned char) *p)) {
      /* skip whitespace */
      ++p;
    }

    /* Check for numbers. */
    if (p < end && isdigit((unsigned char) *p)) {
      int n = *p - '0';
      while (++p < end && isdigit((unsigned char) *p)) {
        n = 10 * n + (*p - '0');
      }
      res_list_append(list, n);
      found = true;
    }
  }
  if (res_list_size(list) == 0) {
    res_list_destroy(list);
    res_parse_throw();
  } else {
    res_stream_take(stream, p - data);
  }
 
  return list;
}

token_t res_expect_string(stream_t* stream, char** result)
{
  int length = 0;
  char* data = res_stream_incoming(stream, &length);
  char* end = data + length;
  char* p = data;
  char* start;

  while (p < end && isspace((unsigned char) *p)) {
    /* skip whitespace */
    ++p;
  }
  start = p;
  while (p < end && (isalnum((unsigned char) *p) || *p == '.' || *p == '-')) {
    ++p;
  }
  if (p > start) {
    if (result) {
      int amount = (p - start);
      char* copy = xmalloc(1 + amount);
      memcpy(copy, start, amount);
      copy[amount] = '\0';
      *result = copy;
    }
    res_stream_take(stream, p - data);
    return String;
  } else {
    return Notoken;
  }
}

void res_parse_expect(stream_t* stream, token_t expect, void* result)
{
  token_t got = Notoken;

  if (expect == String) {
    got = res_expect_string(stream, (char **) result);
  } else {
    got = res_parse_token(stream, result);
  }

  if (got != expect) {
    res_info("%s: Expected %s, got %s\n", __func__,
             res_token_string(expect), res_token_string(got));
    res_parse_throw();
  }
}

