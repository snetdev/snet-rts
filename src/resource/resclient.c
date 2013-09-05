#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/param.h>
#include <setjmp.h>
#include <ctype.h>

#include "resdefs.h"
#include "resparse.h"

client_t* res_client_create(int bit, int fd)
{
  client_t *client = xnew(client_t);
  res_stream_init(&client->stream, fd);
  client->bit = bit;
  client->access = 0;
  client->local_workload = 0;
  client->remote_workload = 0;
  client->local_granted = 0;
  client->local_accepted = 0;
  client->local_revoked = 0;
  client->local_assigning = BITMAP_ZERO;
  client->local_grantmap = BITMAP_ZERO;
  client->local_revoking = BITMAP_ZERO;
  client->rebalance = false;
  return client;
}

void res_client_destroy(client_t* client)
{
  res_stream_done(&client->stream);
  xdel(client);
}

void res_client_expect(client_t* client, token_t expect)
{
  token_t got = res_parse_token(&client->stream, NULL);
  if (got != expect) {
    res_info("Expected %s, got %s\n",
             res_token_string(expect), res_token_string(got));
    res_parse_throw();
  }
}

void res_client_number(client_t* client, int* number)
{
  const int expect = Number;
  token_t got = res_parse_token(&client->stream, number);
  if (got != expect) {
    res_info("Expected %s, got %s\n",
             res_token_string(expect), res_token_string(got));
    res_parse_throw();
  }
}

void res_client_reply(client_t* client, const char* fmt, ...)
{
  const int max_reserve = 1024;
  const char *str = fmt, *arg;
  va_list ap;
  int size = 0;
  char* data = res_stream_outgoing(&client->stream, &size);
  char* start = data;
  char* end = data + size;

  va_start(ap, fmt);
  for (str = fmt; *str; ++str) {
    if (start + 50 > end) {
      res_stream_appended(&client->stream, start - data);
      res_stream_reserve(&client->stream, max_reserve);
      data = res_stream_outgoing(&client->stream, &size);
      assert(size >= max_reserve);
      start = data;
      end = data + size;
    }
    if (*str != '%') {
      *start++ = *str;
      *start = '\0';
    } else if (str[1] == '\0') {
      assert(false);
    } else {
      switch (*++str) {
        case 's': {
          for (arg = va_arg(ap, char*); *arg; ++arg) {
            if (start + 10 > end) {
              res_stream_appended(&client->stream, start - data);
              res_stream_reserve(&client->stream, max_reserve);
              data = res_stream_outgoing(&client->stream, &size);
              assert(size >= max_reserve);
              start = data;
              end = data + size;
            }
            *start++ = *arg;
            *start = '\0';
          }
        } break;
        case 'd': {
          char *first = start, *last;
          int n = va_arg(ap, int);
          assert(n >= 0);
          /* First convert the number and store in reverse order. */
          do {
            *start++ = ((n % 10) + '0');
            *start = '\0';
            n /= 10;
          } while (n);
          /* Then reverse the order of the stored digits. */
          for (last = start - 1; last > first; --last, ++first) {
            char save = *first; *first = *last; *last = save;
          }
        } break;
        default: assert(false);
      }
    }
  }
  va_end(ap);

  res_stream_appended(&client->stream, start - data);
}

void res_client_command_list(client_t* client)
{
  // Currently support only the local system
  res_client_reply(client, "%s", "{ systems 0 }\n");
}

void res_client_command_topology(client_t* client)
{
  int id = 0;
  res_client_number(client, &id);
  if (id) {
    res_info("Invalid topology id.\n");
    res_parse_throw();
  } else {
    char *host = res_hostname();
    int len, size = 10*1024;
    char *str = xmalloc(size);
    str[0] = '\0';
    snprintf(str, size, "{ hardware %d host %s children 1 \n", id, host);
    len = strlen(str);
    str = res_resource_string(NULL, str, len, &size);
    len += strlen(str + len);
    if (len + 10 > size) {
      size = len + 10;
      str = xrealloc(str, size);
    }
    snprintf(str + len, size - len, "} \n");
    res_client_reply(client, "%s", str);
    xfree(str);
    xfree(host);
  }
}

void res_client_command_access(client_t* client)
{
  intlist_t* ints = res_parse_intlist(&client->stream);
  int size = res_list_size(ints);
  int i;
  unsigned long access = 0;

  for (i = 0; i < size; i++) {
    int val = res_list_get(ints, i);
    if (val < 0 || val >= 8*sizeof(client->access)) {
      res_list_destroy(ints);
      res_parse_throw();
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
  intlist_t* ints = res_parse_intlist(&client->stream);
  int result = res_accept_procs(client, ints);
  res_list_destroy(ints);
  if (result == -1) {
    res_parse_throw();
  }
}

void res_client_command_return(client_t* client)
{
  intlist_t* ints = res_parse_intlist(&client->stream);
  int result = res_return_procs(client, ints);
  res_list_destroy(ints);
  if (result == -1) {
    res_parse_throw();
  }
}

void res_client_command_quit(client_t* client)
{
  res_parse_throw();
}

void res_client_command_help(client_t* client)
{
  static const char help_text[] =
  "The following commands are supported:\n"
  "{ list }\n"
  "{ topology int }\n"
  "{ access intlist }\n"
  "{ local int }\n"
  "{ remote int }\n"
  "{ accept intlist }\n"
  "{ return intlist }\n"
  "{ quit }\n"
  "{ help }\n"
  "{ state }\n"
  "Where 'intlist' is a space separated list of postive integers.\n"
  "\n";

  res_client_reply(client, "%s", help_text);
}

void res_client_command_state(client_t* client)
{
  res_client_reply(client,
     "{ state \n"
     "  { client client %d local %d remote %d \n"
     "    granted %d accepted %d revoked %d \n"
     "    grantmap %d rebalance %d } \n",
     client->bit, client->local_workload, client->remote_workload,
     client->local_granted, client->local_accepted, client->local_revoked,
     client->local_grantmap, client->rebalance);

  res_client_reply(client, "} \n");
}

void res_client_command(client_t* client)
{
  token_t command = res_parse_token(&client->stream, NULL);
  switch (command) {
    case List:     res_client_command_list(client); break;
    case Topology: res_client_command_topology(client); break;
    case Access:   res_client_command_access(client); break;
    case Local:    res_client_command_local(client); break;
    case Remote:   res_client_command_remote(client); break;
    case Accept:   res_client_command_accept(client); break;
    case Return:   res_client_command_return(client); break;
    case Quit:     res_client_command_quit(client); break;
    case Help:     res_client_command_help(client); break;
    case State:    res_client_command_state(client); break;
    default:
      res_info("Unexpected token %s.\n", res_token_string(command));
      res_parse_throw();
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
    while ((complete = res_parse_complete(&client->stream)) == Complete) {
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

