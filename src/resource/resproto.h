#ifndef RESPROTO_H
#define RESPROTO_H

/* resmain.c */

void pexit(const char *mesg);
void res_warn(const char *fmt, ...);
void res_error(const char *fmt, ...);
void res_info(const char *fmt, ...);
void res_debug(const char *fmt, ...);

/* resmem.c */

void *xmalloc(size_t siz);
void *xcalloc(size_t n, size_t siz);
void *xrealloc(void *p, size_t siz);
void xfree(void *p);
char* xdup(const char *s);

/* resnet.c */

int res_listen_socket(int listen_port);
int res_connect_socket(int connect_port, char *address);

/* resserv.c */

int hwinit(void);


#endif
