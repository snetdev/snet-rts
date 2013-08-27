#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

int res_listen_socket(int listen_port)
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
    memset(&a, 0, sizeof(a));
    a.sin_port = htons(listen_port);
    a.sin_family = AF_INET;
    if (bind(s, (struct sockaddr *) &a, sizeof(a)) == -1) {
        perror("bind");
        close(s);
        return -1;
    }
    listen(s, 10);
    return s;
}

int res_connect_socket(int connect_port, char *address)
{
    struct sockaddr_in a;
    int s;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        close(s);
        return -1;
    }

    memset(&a, 0, sizeof(a));
    a.sin_port = htons(connect_port);
    a.sin_family = AF_INET;

    if (*address < '0' || *address > '9') {
        struct hostent *host = gethostbyname(address);
        if (!host) {
            herror(address);
            close(s);
            return -1;
        }
        a.sin_addr = *(struct in_addr*)host->h_addr;
    }
    else if (!inet_aton(address, (struct in_addr *) &a.sin_addr.s_addr)) {
        perror("bad IP address format");
        close(s);
        return -1;
    }

    if (connect(s, (struct sockaddr *) &a, sizeof(a)) == -1) {
        perror("connect()");
        close(s);
        return -1;
    }
    return s;
}

