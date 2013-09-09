#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "resdefs.h"

int res_nonblocking(int fd, bool nb)
{
  int yes = nb;
  if (ioctl(fd, FIONBIO, (char *)&yes)) {
    perror("ioctl FIONBIO");
    return -1;
  }
  return 0;
}

int res_listen_socket(const char* listen_addr, int listen_port, bool nb)
{
  struct sockaddr_in a;
  int s;
  int yes = 1;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return -1;
  }

  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    perror("setsockopt");
    close(s);
    return -1;
  }

  if (nb && res_nonblocking(s, nb)) {
    close(s);
    return -1;
  }
  if (fcntl(s, F_SETFD, FD_CLOEXEC) == -1) {
    perror("ioctl FD_CLOEXEC");
    close(s);
    return -1;
  }

  memset(&a, 0, sizeof(a));
  a.sin_family = AF_INET;
  a.sin_port = htons(listen_port);

  if (listen_addr == NULL) {
    a.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else if (*listen_addr < '0' || *listen_addr > '9') {
    struct hostent *host = gethostbyname(listen_addr);
    if (!host) {
      herror(listen_addr);
      close(s);
      return -1;
    }
    a.sin_addr = *(struct in_addr *)host->h_addr;
  }
  else if (!inet_aton(listen_addr, (struct in_addr *)&a.sin_addr.s_addr)) {
    perror("bad IP address format");
    close(s);
    return -1;
  }

  if (bind(s, (struct sockaddr *)&a, sizeof(a))) {
    perror("bind");
    close(s);
    return -1;
  }
  listen(s, 2);
  return s;
}

int res_connect_socket(int connect_port, char *address, bool nb)
{
  struct sockaddr_in a;
  int s;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    close(s);
    return -1;
  }

  if (nb && res_nonblocking(s, nb)) {
    close(s);
    return -1;
  }
  if (fcntl(s, F_SETFD, FD_CLOEXEC) == -1) {
    perror("ioctl FD_CLOEXEC");
    close(s);
    return -1;
  }

  memset(&a, 0, sizeof(a));
  a.sin_family = AF_INET;
  a.sin_port = htons(connect_port);

  if (*address < '0' || *address > '9') {
    struct hostent *host = gethostbyname(address);
    if (!host) {
      herror(address);
      close(s);
      return -1;
    }
    a.sin_addr = *(struct in_addr *)host->h_addr;
  }
  else if (!inet_aton(address, (struct in_addr *)&a.sin_addr.s_addr)) {
    perror("bad IP address format");
    close(s);
    return -1;
  }

  if (connect(s, (struct sockaddr *)&a, sizeof(a)) == -1) {
    if (errno != EINPROGRESS) {
      perror("connect()");
      close(s);
      return -1;
    }
  }
  return s;
}

int res_accept_socket(int listen, bool nb)
{
  struct sockaddr_in a;
  socklen_t len = sizeof(a);
  int s, port;
  char *str;

  if ((s = accept(listen, &a, &len)) == -1) {
    perror("accept");
    return -1;
  }
  str = inet_ntoa(a.sin_addr);
  port = ntohs(a.sin_port);
  res_info("Accepting connection from %s:%d.\n", str, port);

  if (nb && res_nonblocking(s, nb)) {
    close(s);
    return -1;
  }
  if (fcntl(s, F_SETFD, FD_CLOEXEC) == -1) {
    perror("ioctl FD_CLOEXEC");
    close(s);
    return -1;
  }

  return s;
}

void res_socket_close(int sock)
{
  close(sock);
}

int res_socket_receive(int sock, char* buf, int count)
{
  return read(sock, buf, count);
}

int res_socket_send(int sock, const char* buf, int count)
{
  return write(sock, buf, count);
}

char* res_hostname(void)
{
  struct addrinfo hints, *info = NULL;
  int gai_result;
  char* canonical = NULL;
  char hostname[HOST_NAME_MAX];

  gethostname(hostname, HOST_NAME_MAX);
  hostname[HOST_NAME_MAX - 1] = '\0';

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  hints.ai_protocol = IPPROTO_IP;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  if ((gai_result = getaddrinfo(hostname, "ssh", &hints, &info)) != 0) {
    res_error("getaddrinfo: %s\n", gai_strerror(gai_result));
  } else {
    canonical = xstrdup(info->ai_canonname);
    freeaddrinfo(info);
  }

  return canonical;
}

