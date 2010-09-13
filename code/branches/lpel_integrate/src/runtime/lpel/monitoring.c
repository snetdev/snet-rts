
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "stream.h"

#include "monitoring.h"
#include "timing.h"

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



void MonitoringInit(monitoring_t *mon, int worker_id, int flags)
{
  char fname[11];
  int cnt_written;
  cnt_written = snprintf(fname, 11, "out%03d.log", worker_id);
  assert(cnt_written==10);

  /* copy flags */
  mon->flags = flags;

  /* open logfile */
  mon->outfile = fopen(fname, "w");
  assert( mon->outfile != NULL);
}

void MonitoringCleanup(monitoring_t *mon)
{
  int ret;
  ret = fclose(mon->outfile);
  assert(ret == 0);
}

void MonitoringPrint(monitoring_t *mon, task_t *t)
{
  int ret;
  char buf[PRINT_TS_BUFLEN];

  if ( mon->flags == MONITORING_NONE ) return;

  /* determine if task is to be monitored */
  if ( BIT_IS_SET(t->attr.flags, TASK_ATTR_MONITOR) ||
      IS_FLAG(MONITORING_ALLTASKS) ) {

    PRINT_TS(&t->times.stop, buf);

    fprintf( mon->outfile,
        "%s wid %d tid %lu disp %lu st %c ",
        buf, t->owner, t->uid, t->cnt_dispatch, t->state
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
    ret = fflush(mon->outfile);
    assert(ret == 0);
  } /* end if */
}

void MonitoringDebug(monitoring_t *mon, const char *fmt, ...)
{
  timing_t ts;
  char buf[PRINT_TS_BUFLEN];
  va_list ap;

  /* print current timestamp */
  TIMESTAMP(&ts);
  PRINT_TS(&ts,buf);
  fprintf(mon->outfile, "%s ", buf);

  va_start(ap, fmt);
  vfprintf(mon->outfile, fmt, ap);
  va_end(ap);

  fflush(mon->outfile);
}
