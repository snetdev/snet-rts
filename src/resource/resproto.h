#ifndef RESPROTO_H
#define RESPROTO_H

/* reslist.c */

intlist_t* res_list_create(void);
void res_list_set(intlist_t* list, int key, int val);
int res_list_get(intlist_t* list, int key);
void res_list_destroy(intlist_t* list);
int res_list_size(intlist_t* list);
void res_list_append(intlist_t* list, int val);

/* resloop.c */

const char *res_token_string(token_t token);
client_t* res_client_create(int fd);
void res_client_destroy(client_t* client);
void res_client_throw(void);
token_t res_client_token(client_t* client, int* number);
void res_client_expect(client_t* client, token_t expect);
void res_client_number(client_t* client, int* number);
void res_client_reply(client_t* client, const char* fmt, ...);
intlist_t* res_client_intlist(client_t* client);
void res_client_command_systems(client_t* client);
void res_client_command_topology(client_t* client);
void res_client_command_access(client_t* client);
void res_client_command_local(client_t* client);
void res_client_command_remote(client_t* client);
void res_client_command_accept(client_t* client);
void res_client_command_return(client_t* client);
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

/* resstream.c */

void res_buffer_init(buffer_t* buf);
void res_buffer_done(buffer_t* buf);
void res_buffer_appended(buffer_t* buf, int amount);
void res_buffer_reserve(buffer_t* buf, int amount);
int res_buffer_stored(buffer_t* buf);
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
void res_stream_reserve(stream_t* stream, int amount);
void res_stream_appended(stream_t* stream, int amount);
char* res_stream_outgoing(stream_t* stream, int* amount);

/* restopo.c */

char* res_topo_string(res_t* obj, char* str, int len, int *size);
void res_hw_init(void);
const char *res_kind_string(int kind);


#endif
