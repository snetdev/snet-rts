
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "stream.h"

#include "monitoring.h"
#include "scheduler.h"
#include "timing.h"
#include "task.h"
#include "lpel.h"

#define IS_FLAG(v)  ( (mon->flags & (v)) == (v) )

#define PRINT_TS_BUFLEN 20

/**
 * Print a timestamp into a char[] buffer
 *
 * @param ts   pointer to timing_t
 * @param buf  pointer to char[]
 * @pre        buf has at least PRINT_TS_BUFLEN chars space
 */
#define PRINT_TS(ts, buf) do {\
  (void) snprintf((buf), PRINT_TS_BUFLEN, "%lu%09lu", \
      (unsigned long) (ts)->tv_sec, (ts)->tv_nsec \
      ); \
} while (0)


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
  char buf[PRINT_TS_BUFLEN];
  va_list ap;

  if ( env->mon.outfile == NULL) return;

  /* print current timestamp */
  TIMESTAMP(&ts);
  PRINT_TS(&ts,buf);
  fprintf( env->mon.outfile, "%s ", buf);

  va_start(ap, fmt);
  vfprintf( env->mon.outfile, fmt, ap);
  va_end(ap);

  //fflush( env->mon.outfile);
}


void MonTaskEvent( task_t *t, const char *fmt, ...)
{
  //monitoring_t *mon = &t->sched_context->env.mon;

}



void MonTaskPrint( lpelthread_t *env, task_t *t)
{
  char buf[PRINT_TS_BUFLEN];
  monitoring_t *mon = &env->mon;

  if (( env->mon.outfile == NULL) 
      || ( mon->flags == MONITORING_NONE )) {
    return;
  }

  /* determine if task is to be monitored */
  if ( BIT_IS_SET(t->attr.flags, TASK_ATTR_MONITOR) ||
      IS_FLAG(MONITORING_ALLTASKS) ) {

    PRINT_TS(&t->times.stop, buf);

    fprintf( mon->outfile,
        "%s tid %lu disp %lu st %c ",
        buf, t->uid, t->cnt_dispatch, t->state
        );

    switch (t->state) {
      case TASK_WAITING:
        fprintf( mon->outfile, "on %c:%p ", t->wait_on, t->wait_s );
        break;
      case TASK_ZOMBIE:
        PRINT_TS(&t->times.creat, buf);
        fprintf( mon->outfile, "creat %s ", buf );
        break;
      default: /*NOP*/;
    }

    if ( IS_FLAG( MONITORING_TIMES ) ) {
      /* stop time was used for timestamp output */
      /* fprintf( mon->outfile, "stop %s ", buf ); */
      /* start time */
      PRINT_TS(&t->times.start, buf);
      fprintf( mon->outfile, "start %s ", buf );
    }

    if ( IS_FLAG( MONITORING_STREAMINFO ) ) {
      StreamPrintDirty( t, mon->outfile);
    }

    fprintf(mon->outfile, "\n");
    //fflush(mon->outfile);
  } /* end if */
}
