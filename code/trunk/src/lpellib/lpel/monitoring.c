
#include "monitoring.h"


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "arch/timing.h"

#include "threading.h"
#include "worker.h"
#include "task.h"
#include "stream.h"


#define MON_NAME_MAXLEN 20

struct monitoring_t {
  FILE *outfile;
  bool  print_schedctx;
};




#define _MON_FNAME_MAXLEN   (MON_NAME_MAXLEN + 12)
monitoring_t *_LpelMonitoringCreate( int node, char *name)
{
  monitoring_t *mon;
  char monname[MON_NAME_MAXLEN+1];
  char filename[_MON_FNAME_MAXLEN+1];

  strncpy( monname, name, MON_NAME_MAXLEN+1);
  monname[MON_NAME_MAXLEN]= '\0';


  mon = (monitoring_t *) malloc( sizeof(monitoring_t));
  if (node >= 0) {
    (void) snprintf(filename, _MON_FNAME_MAXLEN+1, "mon_n%02d_%s.log", node, monname);
  } else {
    (void) snprintf(filename, _MON_FNAME_MAXLEN+1, "mon_%s.log", monname);
  }
  filename[_MON_FNAME_MAXLEN] = '\0';

  /* open logfile */
  mon->outfile = fopen(filename, "w");
  assert( mon->outfile != NULL);

  /* default values */
  mon->print_schedctx = false;

  return mon;
}


void _LpelMonitoringDestroy( monitoring_t *mon)
{
  if ( mon->outfile != NULL) {
    int ret;
    ret = fclose( mon->outfile);
    assert(ret == 0);
  }

  free( mon);
}




/**
 * Print a time in nsec
 */
static inline void PrintTiming( const timing_t *t, FILE *file)
{
  if (t->tv_sec == 0) {
    (void) fprintf( file, "%lu ", t->tv_nsec );
  } else {
    (void) fprintf( file, "%lu%09lu ",
        (unsigned long) t->tv_sec, t->tv_nsec
        );
  }
}

static void DirtySDPrint( lpel_stream_desc_t *sd, void *arg)
{
  FILE *file = (FILE *)arg;

  (void) fprintf( file,
      "%u,%c,%c,%lu,%c%c%c;",
      sd->sid, sd->mode, sd->state, sd->counter,
      ( sd->event_flags & STDESC_WAITON) ? '?':'-',
      ( sd->event_flags & STDESC_WOKEUP) ? '!':'-',
      ( sd->event_flags & STDESC_MOVED ) ? '*':'-'
      );
}


static void PrintWorkerCtx( workerctx_t *wc, FILE *file)
{
  if ( wc->wid < 0) {
    (void) fprintf(file, "loop %u ", wc->loop);
  } else {
    (void) fprintf(file, "wid %d loop %u ", wc->wid, wc->loop);
  }
}







void _LpelMonitoringDebug( monitoring_t *mon, const char *fmt, ...)
{
  timing_t ts;
  va_list ap;

  if ( (_lpel_global_config.worker_dbg != 1) || !mon || mon->outfile == NULL) return;

  /* print current timestamp */
  TIMESTAMP(&ts);
  PrintTiming( &ts, mon->outfile);
  fprintf( mon->outfile, "*** ");

  va_start(ap, fmt);
  vfprintf( mon->outfile, fmt, ap);
  fflush(mon->outfile);
  va_end(ap);
}




void _LpelMonitoringOutput( monitoring_t *mon, lpel_task_t *t)
{
  FILE *file = mon->outfile;

  if ( file == NULL) return;

  /* timestamp with task stop time */
  PrintTiming( &t->times.stop, file);

  if ( mon->print_schedctx) {
    PrintWorkerCtx( t->worker_context, file);
  }
  

  /* print general info: name, disp.cnt, state */
  fprintf( file,
      "tid %u disp %lu ",
      t->uid, t->cnt_dispatch
      );

  if ( t->state==TASK_BLOCKED) {
    fprintf( file, "st B%c ", t->blocked_on);
  } else {
    fprintf( file, "st %c ", t->state);
  }

  /* print times */
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES) ) {
    timing_t diff;
    TimingDiff( &diff, &t->times.start, &t->times.stop);
    fprintf( file, "et ");
    PrintTiming( &diff , file);
    if ( t->state == TASK_ZOMBIE) {
      fprintf( file, "creat ");
      PrintTiming( &t->times.creat, file);
    }
  }

  /* print stream info */
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_STREAMS) ) {
    fprintf( file,"[" );
    _LpelStreamResetDirty( t, DirtySDPrint, file);
    fprintf( file,"] " );
  }

  fprintf( file, "\n");
  fflush( file);
}

