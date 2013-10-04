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
#include "resserver.h"
#include "resource.h"
#include "restopo.h"

typedef struct server server_t;
typedef enum server_state server_state_t;

static void res_server_reply(server_t* server, const char* fmt, ...);

int res_server_get_socket(server_t* server)
{
  return server->stream.fd;
}

int res_server_get_granted(server_t* server)
{
  return server->granted - server->revoked;
}

void res_server_get_revoke_mask(server_t* server, bitmap_t* mask)
{
  *mask = server->revokemask;
}

int res_server_get_local(server_t* server)
{
  return server->local;
}

int res_server_allocate_proc(server_t* server)
{
  bitmap_t availmask = BITMAP_AND(server->grantmask, BITMAP_NOT(
                           BITMAP_OR(server->assignmask, server->revokemask)));
  int i;
  for(i = 0; i < MAX_BIT; ++i) {
    if (HAS(availmask, i)) {
      break;
    }
  }
  if (i >= MAX_BIT) {
    res_error("[%s]: No proc.\n", __func__);
  } else {
    SET(server->assignmask, i);
  }
  return i;
}

void res_server_release_proc(server_t* server, int proc)
{
  assert(HAS(server->grantmask, proc));
  assert(HAS(server->assignmask, proc));
  CLR(server->assignmask, proc);
  if (HAS(server->revokemask, proc)) {
    res_debug("Returning %d %d.\n", LOCAL_HOST, proc);
    res_server_reply(server, "{ return %d %d } ", LOCAL_HOST, proc);
    CLR(server->revokemask, proc);
    CLR(server->grantmask, proc);
    server->granted -= 1;
    server->revoked -= 1;
    assert(server->granted >= 0);
    assert(server->revoked >= 0);
    res_server_flush(server);
  }
}

static void res_server_change_state(server_t* s, server_state_t A, server_state_t B)
{
  if (s->state != A) {
    res_error("[%s]: Expected state %d, found state %d, intended state %d.\n",
              __func__, A, s->state, B);
  } else {
    s->state = B;
  }
}

static void res_server_expect_state(server_t* server, server_state_t state)
{
  if (server->state != state) {
    res_error("[%s]: Expected state %d, found state %d.\n",
              __func__, state, server->state);
  }
}

static int res_server_write(server_t* server)
{
  return res_stream_write(&server->stream);
}

static bool res_server_writing(server_t* server)
{
  return res_stream_writing(&server->stream);
}

static void res_server_reply(server_t* server, const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  res_stream_reply(&server->stream, fmt, ap);
  va_end(ap);
}

void res_server_set_local(server_t* server, int local)
{
  if (server->local != local) {
    server->local = local;
    res_debug("Send local %d\n", server->local);
    res_server_reply(server, "{ local %d }", server->local);
    res_server_flush(server);
  }
}

static void res_server_expect(server_t* server, token_t expect)
{
  res_parse_expect(&server->stream, expect, NULL);
}

static void res_server_number(server_t* server, int* number)
{
  const int expect = Number;
  res_parse_expect(&server->stream, expect, number);
}

static void res_server_command_systems(server_t* server)
{
  intlist_t* ints = res_parse_intlist(&server->stream);
  int size = res_list_size(ints);
  int i;

  for (i = 0; i < size; ++i) {
    int val = res_list_get(ints, i);
    if (val < 0 || val >= NUM_BITS) {
      res_list_destroy(ints);
      res_parse_throw();
    } else {
      SET(server->systems, val);
    }
  }
  res_list_destroy(ints);

  res_server_change_state(server, ServerListSent, ServerListRcvd);

  if (NOT(server->systems, 0)) {
    res_warn("[%s]: Received systems has no local!\n", __func__);
    res_parse_throw();
  }

  SET(server->access, 0);

  res_debug("Send access 0\n");
  res_server_reply(server, "%s", "{ access 0 }");
  res_debug("Send topology 0\n");
  res_server_reply(server, "%s", "{ topology 0 }");

  res_server_change_state(server, ServerListRcvd, ServerTopoSent);
}

static void res_server_command_system(server_t* server)
{
  host_t* host = NULL;

  res_parse_system(&server->stream, &host);

  assert(host->index == 0);
  res_topo_add_host(host);

  res_server_change_state(server, ServerTopoSent, ServerTopoRcvd);
}

static void res_server_command_grant(server_t* server)
{
  int sysid;
  int len = 0;
  char* p = res_stream_incoming(&server->stream, &len);
  char* q = strchr(p, '}');
  len = q - p;

  res_debug("Got Grant %*.*s.\n", len, len, p);

  res_server_expect_state(server, ServerTopoRcvd);
  res_server_number(server, &sysid);

  if (NOT(server->access, sysid)) {
    res_error("[%s]: Invalid sysid\n", __func__, sysid);
  } else {
    intlist_t* ints = res_parse_intlist(&server->stream);
    int i, na, size = res_list_size(ints);
    res_server_reply(server, "{ accept %d ", sysid);
    for (i = 0; i < size; ++i) {
      int proc = res_list_get(ints, i);
      assert(NOT(server->grantmask, proc));
      SET(server->grantmask, proc);
      server->granted += 1;
      if (HAS(server->revokemask, proc)) {
        CLR(server->revokemask, proc);
        server->revoked -= 1;
        assert(server->revoked >= 0);
      }
      res_server_reply(server, "%d ", proc);
    }
    res_server_reply(server, "} \n");
    for (i = na = 0; i < MAX_BIT; ++i) {
      na += HAS(server->assignmask, i) != 0;
    }
    res_debug("Granted %d, total %d, assigned %d.\n", size, server->granted, na);
  }
}

static void res_server_command_revoke(server_t* server)
{
  int sysid;
  int len = 0;
  char* p = res_stream_incoming(&server->stream, &len);
  char* q = strchr(p, '}');
  len = q - p;

  res_debug("Got Revoke %*.*s.\n", len, len, p);

  res_server_expect_state(server, ServerTopoRcvd);
  res_server_number(server, &sysid);

  if (NOT(server->access, sysid)) {
    res_error("[%s]: Invalid sysid\n", __func__, sysid);
  } else {
    intlist_t* ints = res_parse_intlist(&server->stream);
    int i, size = res_list_size(ints);
    bool replying = false;
    for (i = 0; i < size; ++i) {
      int proc = res_list_get(ints, i);
      assert(HAS(server->grantmask, proc));
      if (HAS(server->assignmask, proc)) {
        SET(server->revokemask, proc);
        server->revoked += 1;
      } else {
        CLR(server->grantmask, proc);
        server->granted -= 1;
        assert(server->granted >= 0);
        if (replying == false) {
          res_server_reply(server, "{ return %d ", sysid);
          replying = true;
        }
        res_server_reply(server, "%d ", proc);
      }
    }
    if (replying) {
      res_server_reply(server, "} \n");
    }
    res_debug("Revoked %d, remain %d, grants %d.\n",
              size, server->revoked, server->granted);
  }
}

static void res_server_command(server_t* server)
{
  int command = res_parse_token(&server->stream, NULL);
  switch (command) {
    case Systems: res_server_command_systems(server); break;
    case System: res_server_command_system(server); break;
    case Grant: res_server_command_grant(server); break;
    case Revoke: res_server_command_revoke(server); break;
    default:
      res_warn("[%s]: Unexpected token %s.\n", __func__, res_token_string(command));
      res_parse_throw();
      break;
  }
}

static void res_server_parse(server_t* server)
{
  res_server_expect(server, Left);
  res_server_command(server);
  res_server_expect(server, Right);
}

static int res_server_process(server_t* server)
{
  if (setjmp(res_parse_exception_context)) {
    res_error("[%s]: Parse error\n", __func__);
    return -1;
  } else {
    res_server_parse(server);
    return 0;
  }
}

int res_server_flush(server_t* server)
{
  while (res_server_writing(server)) {
    if (res_server_write(server) < 0) {
      return -1;
    }
  }
  return 0;
}

int res_server_read(server_t* server)
{
  enum { Complete = 1, Incomplete = 0, Failure = -1 };

  if (res_stream_read(&server->stream) == Failure) {
    return Failure;
  } else {
    int complete;
    while ((complete = res_parse_complete(&server->stream)) == Complete) {
      if (res_server_process(server) == Failure) {
        return Failure;
      }
    }
    if (complete == Failure) {
      return Failure;
    }
    res_server_flush(server);
  }
  return 0;
}

void res_server_setup(server_t* server)
{
  res_debug("Send list\n");
  res_server_reply(server, "%s", "{ list }");
  res_server_change_state(server, ServerInit, ServerListSent);

  while (server->state < ServerTopoRcvd) {
    const int sock = server->stream.fd;
    int num;

    fd_set rset, wset, *wptr = NULL;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(sock, &rset);
    if (res_server_writing(server)) {
      FD_SET(sock, &wset);
      wptr = &wset;
    }

    num = select(sock + 1, &rset, wptr, NULL, NULL);
    if (num == -1) {
      res_perror("select");
      break;
    }
    assert(num > 0);

    if (wptr && FD_ISSET(sock, wptr)) {
      int r = res_server_write(server);
      if (r < 0) {
        break;
      }
      assert(r > 0);
    }
    if (FD_ISSET(sock, &rset)) {
      int r = res_server_read(server);
      if (r <= 0) {
        break;
      }
    }
  }
}

server_t* res_server_create(int fd)
{
  server_t *server = xnew(server_t);
  res_stream_init(&server->stream, fd);
  ZERO(server->systems);
  ZERO(server->access);
  server->state = ServerInit;
  server->local = 0;
  server->granted = 0;
  server->revoked = 0;
  ZERO(server->grantmask);
  ZERO(server->assignmask);
  ZERO(server->revokemask);
  return server;
}

void res_server_destroy(server_t* server)
{
  if (server) {
    res_stream_done(&server->stream);
    xdel(server);
  }
}

server_t* res_server_connect(const char* addr, int port)
{
  server_t* server = NULL;
  int fd = res_connect_socket(port, addr, false);
  if (fd >= 0) {
    server = res_server_create(fd);
    res_server_setup(server);
  }

  if (!server) {
    res_warn("%s:[%s]: Failed to connect to resource server %s on port %d.\n",
             res_get_program_name(), __func__, addr, port);
  }
  return server;
}

void res_server_interact(server_t* server)
{
  res_server_process(server);
}

