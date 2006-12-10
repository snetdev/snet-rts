

#ifndef DEBUG_H 
#define DEBUG_H

#include <string.h>
#include <buffer.h>



typedef struct debug debug_t;


extern debug_t *DBGinit( int num);

extern void DBGdestroy( debug_t *dbg);

extern void DBGsetName( debug_t *dbg, snet_buffer_t *buf, char *name);

extern void DBGregisterBuffer( debug_t *dbg, snet_buffer_t *buf);

extern void DBGtraceGet( debug_t *dbg, snet_buffer_t *buf);
#endif

