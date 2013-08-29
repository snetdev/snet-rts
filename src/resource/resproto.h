#ifndef RESPROTO_H
#define RESPROTO_H

/* resloop.c */

client_t* res_client_create(int fd);
void res_client_destroy(client_t* client);
int res_client_catch(void);
void res_client_throw(void);
char res_client_char(client_t* client);
void res_client_consume_char(client_t* client, char sym);
void res_client_command(client_t* client);
int res_client_process(client_t* client);
int res_client_complete(client_t* client);
int res_client_read(client_t* client);
int res_client_write(client_t* client);
bool res_client_writing(client_t* client);
void res_loop(int port);

/* resmain.c */

void pexit(const char *mesg);
void res_warn(const char *fmt, ...);
void res_error(const char *fmt, ...);
void res_info(const char *fmt, ...);
void res_debug(const char *fmt, ...);

/* resmap.c */

intmap_t* res_map_create(void);
void res_map_add(intmap_t* map, int key, void* val);
void* res_map_get(intmap_t* map, int key);
void res_map_del(intmap_t* map, int key);
void res_map_destroy(intmap_t* map);
void res_map_iter_new(intmap_t* map, int* iter);
void* res_map_iter_next(intmap_t* map, int* iter);

/* resmem.c */

void *xmalloc(size_t siz);
void *xcalloc(size_t n, size_t siz);
void *xrealloc(void *p, size_t siz);
void xfree(void *p);
void xdel(void *p);
char* xdup(const char *s);

/* resnet.c */

int res_nonblocking(int fd, bool nb);
int res_listen_socket(int listen_port, bool nb);
int res_connect_socket(int connect_port, char *address, bool nb);
int res_accept_socket(int listen, bool nb);

/* resserv.c */

void res_hw_init(void);
const char *res_kind_string(int kind);

/* resstream.c */

void res_buffer_init(buffer_t* buf);
void res_buffer_done(buffer_t* buf);
void res_buffer_reserve(buffer_t* buf, int amount);
int res_buffer_avail(buffer_t* buf);
char* res_buffer_data(buffer_t* buf);
void res_buffer_take(buffer_t* buf, int amount);
int res_buffer_read(buffer_t* buf, int fd, int amount);
int res_buffer_write(buffer_t* buf, int fd, int amount);
void res_stream_init(stream_t* stream, int fd);
void res_stream_done(stream_t* stream);
int res_stream_read(stream_t* stream);
int res_stream_write(stream_t* stream);
bool res_stream_writing(stream_t* stream);
char* res_stream_incoming(stream_t* stream, int* amount);
void res_stream_take(stream_t* stream, int amount);


#endif
