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

typedef enum token token_t;

jmp_buf res_parse_exception_context;

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
  TOK(Quit)
  TOK(Help)
  TOK(State)
  "Unknown";
#undef NAME
#undef TOK
}

void res_parse_throw(void)
{
  longjmp(res_parse_exception_context, 1);
}

token_t res_parse_token(stream_t* stream, int* number)
{
  token_t token = Notoken;
  int length = 0;
  char* data = res_stream_incoming(stream, &length);
  char* end = data + length;
  char* p;

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
      if (number != NULL) {
        *number = n;
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
      { Quit,     "quit",     4 },
      { Help,     "help",     4 },
      { State,    "state",    5 },
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
    res_parse_throw();
  }

  res_stream_take(stream, p - data);

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

