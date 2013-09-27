#ifndef RESPROTO_H
#define RESPROTO_H

/* resbalance.c */


/* A client returns all its resources to the server (when it exits). */
void res_release_client(client_t* client);

/* A client confirms a previous processor grant. */
int res_accept_procs(client_t* client, intlist_t* ints);

/* A client returns previously granted processors. */
int res_return_procs(client_t* client, intlist_t* ints);

/* Compare the load of two clients, descending. */
int res_client_compare_local(const void *p, const void *q);

/* Each task can be run on a dedicated core. */
void res_rebalance_cores(intmap_t* map);
void res_rebalance_procs(intmap_t* map);
void res_rebalance_proportional(intmap_t* map);
void res_rebalance_minimal(intmap_t* map);
void res_rebalance(intmap_t* map);

/* resclient.c */

client_t* res_client_create(int bit, int fd);
void res_client_destroy(client_t* client);
void res_client_expect(client_t* client, token_t expect);
void res_client_number(client_t* client, int* number);
void res_client_reply(client_t* client, const char* fmt, ...);
void res_client_command_list(client_t* client);
void res_client_command_topology(client_t* client);
void res_client_command_resources(client_t* client);
void res_client_command_access(client_t* client);
void res_client_command_local(client_t* client);
void res_client_command_remote(client_t* client);
void res_client_command_accept(client_t* client);
void res_client_command_return(client_t* client);
void res_client_command_quit(client_t* client);
void res_client_command_help(client_t* client);
void res_client_command_state(client_t* client);
void res_client_command_shutdown(client_t* client);
void res_client_command(client_t* client);
void res_client_parse(client_t* client);
int res_client_process(client_t* client);
int res_client_write(client_t* client);
bool res_client_writing(client_t* client);
int res_client_read(client_t* client);

/* rescpubind.c */

pid_t res_gettid(void);
int res_bind_physical_proc(int physical_proc);
int res_bind_logical_proc(int logical_proc);

/* rescpuinfo.c */

resource_t* res_cpuinfo_resource_init(void);

/* reshwloc.c */

resource_t* res_hwloc_resource_init(void);

/* reslist.c */

void res_list_init(intlist_t* list);
void res_list_init_max(intlist_t* list, int max);
intlist_t* res_list_create(void);
intlist_t* res_list_create_max(int max);
void res_list_reset(intlist_t* list);
void res_list_move(intlist_t* dest, intlist_t* src);
void res_list_copy(intlist_t* dest, intlist_t* src);
void res_list_set(intlist_t* list, int key, int val);
int res_list_get(intlist_t* list, int key);
void res_list_done(intlist_t* list);
void res_list_destroy(intlist_t* list);
int res_list_size(intlist_t* list);
void res_list_append(intlist_t* list, int val);
void res_list_sort(intlist_t* list, int (*compar)(const void *, const void *));
void res_list_sort_ascend(intlist_t* list);
void res_list_sort_descend(intlist_t* list);
void res_list_iter_init(intlist_t* list, intlist_iter_t* iter);
bool res_list_iter_next(intlist_t* list, intlist_iter_t* iter, int *value);

/* resloop.c */

void res_loop(int listen);
char* res_slurp(void* vp);
void res_parse_slave(int sysid, char* output);
void res_start_slave(int sysid, char* command);
void res_start_slaves(intmap_t* slaves);
void res_catch_sigint(int s);
void res_catch_signals(void);
void res_service(const char* listen_addr, int listen_port, intmap_t* slaves);

/* resmain.c */


/* resmap.c */

intmap_t* res_map_create(void);
int res_map_max(intmap_t* map);
int res_map_count(intmap_t* map);
void res_map_add(intmap_t* map, int key, void* val);
void* res_map_get(intmap_t* map, int key);
void res_map_del(intmap_t* map, int key);
void res_map_destroy(intmap_t* map);
void res_map_apply(intmap_t* map, void (*func)(void *));
void res_map_iter_init(intmap_t* map, int* iter);
void* res_map_iter_next(intmap_t* map, int* iter);

/* resmem.c */

void *xmalloc(size_t siz);
void *xcalloc(size_t n, size_t siz);
void *xrealloc(void *p, size_t siz);
char* xstrdup(const char* str);
void xfree(void *p);
void xdel(void *p);
char* xdup(const char *s);

/* resnet.c */

int res_nonblocking(int fd, bool nb);
int res_listen_socket(const char* listen_addr, int listen_port, bool nb);
int res_connect_socket(int connect_port, const char *address, bool nb);
int res_accept_socket(int listen, bool nb);
void res_socket_close(int sock);
int res_socket_receive(int sock, char* buf, int count);
int res_socket_send(int sock, const char* buf, int count);
char* res_hostname(void);

/* resource.c */

int res_local_cores(void);
int res_local_procs(void);

/*
 * Convert a hierarchical resource topology to a single string.
 * Input parameters:
 *      obj: the resource and its children to convert to string.
 *      str: the destination string where output is appended to.
 *      len: the current length of the output string.
 *      size: the current size of the destination string.
 * Output result: Return the current string and its size.
 * When the string memory is too small then the string must be
 * dynamically reallocated to accomodate the new output.
 * The new size of the string must be communicated via the size pointer.
 */
char* res_resource_object_string(resource_t* obj, char* str, int len, int *size);
void res_relate_resources(resource_t *parent, resource_t *child);
resource_t* res_add_resource(
  resource_t *parent,
  res_kind_t kind,
  int topo_depth,
  int logical_index,
  int physical_index,
  int cache_level);
resource_t* res_resource_init(void);
void res_resource_free(resource_t* res);

/* Convert the kind of resource to a static string. */
const char *res_kind_string(int kind);

/* resparse.c */

const char *res_token_string(int token);
void res_parse_throw(void);
token_t res_parse_string(const char* data, int* length, void* result);
token_t res_parse_token(stream_t* stream, void* result);
int res_parse_complete(stream_t* stream);
intlist_t* res_parse_intlist(stream_t* stream);
token_t res_expect_string(stream_t* stream, char** result);
void res_parse_expect(stream_t* stream, token_t expect, void* result);

/* resserver.c */

int res_server_get_socket(server_t* server);
int res_server_get_granted(server_t* server);
void res_server_get_revoke_mask(server_t* server, bitmap_t* mask);
int res_server_get_local(server_t* server);
int res_server_allocate_proc(server_t* server);
void res_server_release_proc(server_t* server, int proc);
void res_server_set_local(server_t* server, int local);
int res_server_flush(server_t* server);
int res_server_read(server_t* server);
void res_server_setup(server_t* server);
server_t* res_server_create(int fd);
void res_server_destroy(server_t* server);
server_t* res_server_create_option(const char* arg);
void res_server_interact(server_t* server);

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
void res_buffer_append(buffer_t* buf, const char* data, int size);
void res_stream_init(stream_t* stream, int fd);
void res_stream_done(stream_t* stream);
stream_t* res_stream_create(int fd);
void res_stream_destroy(stream_t* stream);
int res_stream_read(stream_t* stream);
int res_stream_write(stream_t* stream);
bool res_stream_writing(stream_t* stream);
char* res_stream_incoming(stream_t* stream, int* amount);
void res_stream_take(stream_t* stream, int amount);
void res_stream_reserve(stream_t* stream, int amount);
void res_stream_appended(stream_t* stream, int amount);
char* res_stream_outgoing(stream_t* stream, int* amount);
stream_t* res_stream_from_string(const char* string);
void res_stream_reply(stream_t* stream, const char* fmt, va_list ap);

/* restopo.c */

void res_topo_create(void);
void res_topo_add_host(host_t *host);
host_t* res_topo_get_host(int sysid);
host_t* res_local_host(void);
resource_t* res_host_get_root(host_t* host);
resource_t* res_local_root(void);
resource_t* res_topo_get_root(int sysid);
void res_topo_get_host_list(intlist_t* list);
host_t* res_host_create(char* hostname, int index, resource_t* root);
void res_host_destroy(host_t* host);
void res_host_dump(host_t* host);
bool res_topo_init(void);
void res_topo_destroy(void);
char* res_system_resource_string(int id);
char* res_system_host_string(int id);
void res_parse_system(stream_t* stream, host_t** host_ptr);
bool res_parse_topology(int sysid, char* text);

/* resutil.c */


/* Return current time in a string as HH:MM:SS.mmm. */
void res_local_time_string(char *buf, size_t size);
const char* res_get_program_name(void);
void res_set_program_name(const char* prog);
bool res_get_debug(void);
void res_set_debug(bool flag);
bool res_get_verbose(void);
void res_set_verbose(bool flag);
void res_perror(const char *mesg);
void res_pexit(const char *mesg);
void res_assert_failed(const char *fn, int ln, const char *msg);
void res_warn(const char *fmt, ...);
void res_error(const char *fmt, ...);
void res_info(const char *fmt, ...);
void res_debug(const char *fmt, ...);


#endif
