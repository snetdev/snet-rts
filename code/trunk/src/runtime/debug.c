

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <debug.h>
#include <memfun.h>





struct debug {
  pthread_mutex_t *dbg_mutex;
  int num;
  int current;
  snet_buffer_t **buffers;
  pthread_t **threads;
  char **names;
};


extern debug_t *DBGinit( int num) {
  
  int i;
  debug_t *dbg;

  dbg = SNetMemAlloc( sizeof( debug_t));
  dbg->dbg_mutex = SNetMemAlloc( sizeof(pthread_mutex_t));
  pthread_mutex_init( dbg->dbg_mutex, NULL);

  dbg->num = num;
  dbg->current = 0;
  dbg->buffers = SNetMemAlloc( num * sizeof( snet_buffer_t*));
  dbg->names = SNetMemAlloc( num * sizeof( char*));
  dbg->threads = SNetMemAlloc( num * sizeof( pthread_t*));

  for( i=0; i<num; i++) {
    dbg->buffers[i] = NULL;
    dbg->names[i] = NULL;
    dbg->threads[i] = NULL;
  }

  return( dbg);
}


extern void DBGdestroy( debug_t *dbg) {

  int i;
  
  for( i=0; i<dbg->num; i++) {
    if( dbg->names[i] != NULL) {
      SNetMemFree( dbg->names[i]);
    }
    else {
      break;
    }
  }
  
  SNetMemFree( dbg->buffers);
  SNetMemFree( dbg->threads);
  SNetMemFree( dbg->names);

  SNetMemFree( dbg);
}


extern void DBGregisterBuffer( debug_t *dbg, snet_buffer_t *buf) {

  int i;
  pthread_mutex_lock( dbg->dbg_mutex);
  i = dbg->current;

  if( i >= dbg->num) {
    printf("\n** Fatal Error ** : No Space Left in Debugging Data Structure!\n\n");
    exit( 1);
  }

  dbg->buffers[i] = buf;
  dbg->current += 1;

  pthread_mutex_unlock( dbg->dbg_mutex);

}


extern void DBGsetName( debug_t *dbg, snet_buffer_t *buf, char *name) {

  int i;
  for( i=0; i<dbg->num; i++) {
    if( dbg->buffers[i] == buf) {
        dbg->names[i] = SNetMemAlloc( ( strlen( name) + 1) * sizeof( char));
        strcpy( dbg->names[i], name);
        break;
    }
  }
  if( i == dbg->num) {
    printf("\n** Fatal Error ** : Assigning of Name Failed!\n\n");
    exit( 1);
  }
}

extern void DBGtraceGet( debug_t *dbg, snet_buffer_t *buf) {

  int i;
  char *name=NULL;
  pthread_mutex_lock( dbg->dbg_mutex);
  for( i=0; i<dbg->num; i++) {
    if( dbg->buffers[i] == buf) {
      name = dbg->names[i];
    }
  }
  if( name == NULL) {
    printf("\n** Fatal Error ** : Could Not Find Buffer\n\n");
   exit( 1);
  }
  printf("Statisfied Request: GET -- Buffer: %s\n",  name);
  pthread_mutex_unlock( dbg->dbg_mutex);
}


