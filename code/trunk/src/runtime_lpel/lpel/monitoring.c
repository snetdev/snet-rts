
#include "monitoring.h"


#ifdef MONITORING_ENABLE

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "stream.h"

#include "scheduler.h"
#include "timing.h"
#include "lpel.h"
#include "task.h"



/**
 * mon_n00_worker01.log
 */
void MonInit( lpelthread_t *env, int flags)
{

  if (env->name != NULL) {
#   define FNAME_MAXLEN   (LPEL_THREADNAME_MAXLEN + 12)
    char fname[FNAME_MAXLEN+1];
    if (env->node >= 0) {
      (void) snprintf(fname, FNAME_MAXLEN+1, "mon_n%02d_%s.log", env->node, env->name);
    } else {
      (void) snprintf(fname, FNAME_MAXLEN+1, "mon_%s.log", env->name);
    }
    fname[FNAME_MAXLEN] = '\0';

    /* open logfile */
    env->mon.outfile = fopen(fname, "w");
    assert( env->mon.outfile != NULL);
  } else {
    env->mon.outfile = NULL;
  }
  /* copy flags */
  env->mon.flags = flags;

}

void MonCleanup( lpelthread_t *env)
{
  if ( env->mon.outfile != NULL) {
    int ret;
    ret = fclose( env->mon.outfile);
    assert(ret == 0);
  }
}



void MonDebug( lpelthread_t *env, const char *fmt, ...)
{
  timing_t ts;
  va_list ap;

  if ( env->mon.outfile == NULL) return;

  /* print current timestamp */
  TIMESTAMP(&ts);
  TimingPrint( &ts, env->mon.outfile);

  va_start(ap, fmt);
  vfprintf( env->mon.outfile, fmt, ap);
  va_end(ap);

#ifdef MON_DO_FLUSH
  fflush( env->mon.outfile);
#endif
}


void MonTaskEvent( task_t *t, const char *fmt, ...)
{
  //monitoring_t *mon = &t->sched_context->env.mon;

}



void MonTaskPrint( monitoring_t *mon, struct schedctx *sc, struct task *t)
{
# define IS_FLAG(v)  ( (mon->flags & (v)) == (v) )
  timing_t ts;
  int flags = 0;
  FILE *file = mon->outfile;

  if (( file == NULL) 
      || ( mon->flags == MONITORING_NONE )) {
    return;
  }

  /* get task stop time */
  ts = t->times.stop;
  TimingPrint( &ts, file);

  //TODO check a flag
  //SchedPrintContext( sc, file);

  if ( IS_FLAG( MONITORING_TIMES ) ) {
    flags |= TASK_PRINT_TIMES;
  }
  if ( IS_FLAG( MONITORING_STREAMINFO ) ) {
    flags |= TASK_PRINT_STREAMS;
  }
  TaskPrint( t, file, flags );

  fprintf( file, "\n");
#ifdef MON_DO_FLUSH
  fflush( file);
#endif
}

#endif /* MONITORING_ENABLE */
