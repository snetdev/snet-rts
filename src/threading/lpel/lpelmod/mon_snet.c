#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "mon_snet.h"

#include "locvec.h"
#include "moninfo.h"

#define PrintTiming(t, file)  PrintTimingNs((t),(file))
#define PrintNormTS(t, file)  PrintNormTSns((t),(file))

static int mon_node = -1;
static int mon_flags = 0;


static const char *prefix = "mon_";
static const char *suffix = ".log";
static const char end_entry = END_LOG_ENTRY;
static const char message_trace_sep = MESSAGE_TRACE_SEPARATOR;
static const char worker_start = WORKER_START_EVENT;
static const char worker_end = WORKER_END_EVENT;
static const char worker_wait = WORKER_WAIT_EVENT;

#define MON_USREVT_BUFSIZE_DELTA 64

typedef struct mon_usrevt_t mon_usrevt_t;


/**
 * Every worker has a monitoring context, which besides
 * the log-file handle contains wait-times (idle-time)
 */
struct mon_worker_t {
	int           wid;         /** worker id */
	FILE         *outfile;     /** where to write the monitoring data to */
	unsigned int  disp;        /** count how often a task has been dispatched */
	lpel_timing_t      wait_current;
	unsigned int wait_cnt;
	lpel_timing_t wait_time;
	lpel_timing_t exec_time;
	struct {
		int cnt, size;
		mon_usrevt_t *buffer;
	} events;         /** user-defined events */
	int flags;
};


/**
 * Every task that is enabled for monitoring has its mon_task_t structure
 * that contains the monitored information, e.g. timing information,
 * a list of dirty-streams, etc
 * It also contains a pointer to the monitoring context of
 * the worker the task is assigned to, which is set in
 * the task_assign() handler MonCbTaskAssign().
 */
struct mon_task_t {
	char name[MON_ENTNAME_MAXLEN];
	mon_worker_t *mw;
	unsigned long tid;
	unsigned long flags; /** monitoring flags */
	struct {
		lpel_timing_t creat; /** task creation time */
		lpel_timing_t start; /** start time of last dispatch */
		lpel_timing_t stop;  /** stop time of last dispatch */
	} times;
	mon_stream_t *dirty_list; /** head of dirty stream list */
	unsigned long last_in_cnt;  /** last counter of an input stream */
	unsigned long last_out_cnt; /** last counter of an output stream */
	char blockon;     /** for convenience: tracking if blocked
                        on read or write or any */
};


/**
 * Each stream of a task that monitors stream information
 * gets assigned a mon_stream_t, that is passed to the
  callbacks upon stream procedures.
 */
struct mon_stream_t {
	mon_task_t   *montask;     /** Invariant: != NULL */
	mon_stream_t *dirty;       /** for maintaining a dirty stream list */
	char          mode;        /** either 'r' or 'w' */
	char          state;       /** one if IOCR */
	unsigned int  sid;         /** copy of the stream uid */
	unsigned long counter;     /** number of items processed */
	unsigned int  strevt_flags;/** events "?!*" */
};


/**
 * Item to log the possible user events of the treading interface
 */
struct mon_usrevt_t {
	lpel_timing_t ts;
	snet_moninfo_t *moninfo;
	unsigned long in_cnt;
	unsigned long out_cnt;
};


/**
 * The state of a stream descriptor
 */
#define ST_INUSE    'I'
#define ST_OPENED   'O'
#define ST_CLOSED   'C'
#define ST_REPLACED 'R'

/*
 * The strevt_flags of a stream descriptor
 */
#define ST_MOVED    (1<<0)
#define ST_WAKEUP   (1<<1)
#define ST_BLOCKON  (1<<2)


/**
 * This special value indicates the end of the dirty list chain.
 * NULL cannot be used as NULL indicates that the SD is not dirty.
 */
#define ST_DIRTY_END   ((mon_stream_t *)-1)




/**
 * Reference timestamp
 */
static lpel_timing_t monitoring_begin = LPEL_TIMING_INITIALIZER;





/*****************************************************************************
 * HELPER FUNCTIONS
 ****************************************************************************/

/**
 * Macros for checking flags
 */
#define FLAG_TIMES(mt)    (mt->flags & SNET_MON_TIME)
#define FLAG_STREAMS(mt)  (mt->flags & SNET_MON_STREAM)
#define FLAG_MESSAGE(mt)   (mt->flags & SNET_MON_MESSAGE)
#define FLAG_TASK(mt)  (mt->flags & SNET_MON_TASK)
#define FLAG_WORKER(mt)  (mt->flags & SNET_MON_WORKER)
#define FLAG_LOAD(mt)	(mt->flags & SNET_MON_LOAD)

/**
 * Print a time in usec
 */
static inline void PrintTimingUs( const lpel_timing_t *t, FILE *file)
{
	if (t->tv_sec == 0) {
		(void) fprintf( file, "%lu ", t->tv_nsec / 1000);
	} else {
		(void) fprintf( file, "%lu%06lu ",
				(unsigned long) t->tv_sec, (t->tv_nsec / 1000)
		);
	}
}

/**
 * Print a time in nsec
 */
static inline void PrintTimingNs( const lpel_timing_t *t, FILE *file)
{
	if (t->tv_sec == 0) {
		(void) fprintf( file, "%lu", t->tv_nsec);
	} else {
		(void) fprintf( file, "%lu%09lu",
				(unsigned long) t->tv_sec, (t->tv_nsec)
		);
	}
}

/**
 * Print a normalized timestamp usec
 */
static inline void PrintNormTSus( const lpel_timing_t *t, FILE *file)
{
	lpel_timing_t norm_ts;

	LpelTimingDiff(&norm_ts, &monitoring_begin, t);
	(void) fprintf( file,
			"%lu%06lu",
			(unsigned long) norm_ts.tv_sec,
			(norm_ts.tv_nsec / 1000)
	);
}

/**
 * Print a normalized timestamp nsec
 */
static inline void PrintNormTSns( const lpel_timing_t *t, FILE *file)
{
	lpel_timing_t norm_ts;

	LpelTimingDiff(&norm_ts, &monitoring_begin, t);
	// to shorten the timestamp
	if (norm_ts.tv_sec == 0) {
		(void) fprintf( file, "%lu", norm_ts.tv_nsec);
	} else {
		(void) fprintf( file, "%lu%09lu",
				(unsigned long) norm_ts.tv_sec, (norm_ts.tv_nsec)
		);
	}

	/*
	 // full timestamp
	 (void) fprintf( file,
			"%lu%09lu ",
			(unsigned long) norm_ts.tv_sec,
			(norm_ts.tv_nsec)
	);*/

}


/**
 * Add a stream monitor object to the dirty list of its task.
 * It is only added to the dirty list once.
 */
static inline void MarkDirty( mon_stream_t *ms)
{
	mon_task_t *mt = ms->montask;
	/*
	 * only add if not dirty yet
	 */
	if ( ms->dirty == NULL ) {
		/*
		 * Set the dirty ptr of ms to the dirty_list ptr of montask
		 * and the dirty_list ptr of montask to ms, i.e.,
		 * insert the ms at the front of the dirty_list.
		 * Initially, dirty_list of montask is empty (ST_DIRTY_END, != NULL)
		 */
		ms->dirty = mt->dirty_list;
		mt->dirty_list = ms;
	}
	/* in every case, copy the counter value */
	switch(ms->mode) {
	case 'r': mt->last_in_cnt  = ms->counter; break;
	case 'w': mt->last_out_cnt = ms->counter; break;
	default: assert(0);
	}
}


/**
 * Print the dirty list of the task: stream trace
 */
static void PrintDirtyList(mon_task_t *mt)
{
	mon_stream_t *ms, *next;
	FILE *file = mt->mw->outfile;

	ms = mt->dirty_list;

	while (ms != ST_DIRTY_END) {
		/* all elements in the dirty list must belong to same task */
		assert( ms->montask == mt );

		/* now print */

		(void) fprintf( file,
				"%u%c%c%lu%c%c%c",
				ms->sid, ms->mode, ms->state, ms->counter,
				( ms->strevt_flags & ST_BLOCKON) ? '?':'-',
						( ms->strevt_flags & ST_WAKEUP) ? '!':'-',
								( ms->strevt_flags & ST_MOVED ) ? '*':'-'
		);


		/* get the next dirty entry, and clear the link in the current entry */
		ms->counter = 0;	//reset for stream counter
		next = ms->dirty;

		/* update/reset states */
		switch (ms->state) {
		case ST_OPENED:
		case ST_REPLACED:
			ms->state = ST_INUSE;
			/* fall-through */
		case ST_INUSE:
			ms->dirty = NULL;
			ms->strevt_flags = 0;
			break;

		case ST_CLOSED:
			/* eventually free the mon_stream_t of the closed stream */
			free(ms);
			break;

		default: assert(0);
		}
		ms = next;
	}


	/* dirty list of task is empty */
	mt->dirty_list = ST_DIRTY_END;
}


/**
 * Print the user events of a task
 */
static void PrintUsrEvt(mon_task_t *mt)
{
	mon_worker_t *mw = mt->mw;
	int i;

	if (!mw) return;

	FILE *file = mw->outfile;

	for (i=0; i<mw->events.cnt; i++) {
		mon_usrevt_t *cur = &mw->events.buffer[i];
		/* print cur */
		if FLAG_TIMES(mt) {
			PrintNormTS(&cur->ts, file);
		}

		SNetMonInfoPrint(file, cur->moninfo);

		fprintf( file, "%c", message_trace_sep);

		/*
		 * According to the treading interface, we destroy the moninfo.
		 */
		SNetMonInfoDestroy(cur->moninfo);
	}
	/* reset */
	mw->events.cnt = 0;
}



/*****************************************************************************
 * CALLBACK FUNCTIONS
 ****************************************************************************/


/**
 * Create a monitoring context (for a worker)
 *
 * @param wid   worker id
 * @return a newly created monitoring context
 */
static mon_worker_t *MonCbWorkerCreate( int wid)
{
	char fname[MON_FNAME_MAXLEN+1];

	mon_worker_t *mon = (mon_worker_t *) malloc( sizeof(mon_worker_t));
	mon->flags = mon_flags;

	mon->wid = wid;



	/* build filename */
	memset(fname, 0, MON_FNAME_MAXLEN+1);
	snprintf( fname, MON_FNAME_MAXLEN,
			"%sn%02d_worker%02d%s", prefix, mon_node, wid, suffix);


	/* open logfile */
	mon->outfile = fopen(fname, "w");
	assert( mon->outfile != NULL);
	(void) fprintf(mon->outfile, "%s%c", LOG_FORMAT_VERSION, END_LOG_ENTRY);

	/* default values */
	mon->disp = 0;
	LpelTimingZero(&mon->wait_current);

	/* statistic info */
	mon->wait_cnt = 0;
	LpelTimingZero(&mon->wait_time);

	/* user events */
	mon->events.cnt = 0;
	mon->events.size = MON_USREVT_BUFSIZE_DELTA;
	mon->events.buffer = malloc( mon->events.size * sizeof(mon_usrevt_t));

	/* start message */
	if ((FLAG_WORKER(mon) && FLAG_TIMES(mon))
			|| FLAG_LOAD(mon)){
		lpel_timing_t tnow;
		LpelTimingNow(&tnow);
		PrintNormTS(&tnow, mon->outfile);
	}

	if ((FLAG_WORKER(mon) || FLAG_LOAD(mon)))
		fprintf( mon->outfile, "%c%c", worker_start, end_entry);


	return mon;
}



static mon_worker_t *MonCbWrapperCreate( mon_task_t *mt)
{
	char fname[MON_FNAME_MAXLEN+1];

	mon_worker_t *mon = (mon_worker_t *) malloc( sizeof(mon_worker_t));
	mon->wid = -1;
	mon->flags = mon_flags;

	/* build filename */
	memset(fname, 0, MON_FNAME_MAXLEN+1);
	if (strlen(mt->name)>0) {
		snprintf( fname, MON_FNAME_MAXLEN,
				"%sn%02d_%s%s", prefix, mon_node, mt->name, suffix);
	} else {
		snprintf( fname, MON_FNAME_MAXLEN,
				"%swrapper%02lu%s", prefix, mt->tid, suffix);
	}

	/* open logfile */
	mon->outfile = fopen(fname, "w");
	assert( mon->outfile != NULL);

	/* default values */
	mon->disp = 0;
	LpelTimingZero(&mon->wait_current);

	/* user events */
	mon->events.size = 0;
	mon->events.cnt = 0;
	mon->events.buffer = NULL;

	/* start message */
	if (FLAG_WORKER(mon)){
		if (FLAG_TIMES(mon)) {
			lpel_timing_t tnow;
			LpelTimingNow(&tnow);
			PrintNormTS(&tnow, mon->outfile);
		}


		fprintf(mon->outfile, "%c%c", worker_start, end_entry);
	}
	return mon;
}



static void printStatistic(mon_worker_t *mon){
	fprintf(mon->outfile, "WC%dWT", mon->wait_cnt);
	PrintTiming( &mon->wait_time, mon->outfile);

}
/**
 * Destroy a monitoring context
 *
 * @param mon the monitoring context to destroy
 */
static void MonCbWorkerDestroy( mon_worker_t *mon)
{

	if ((FLAG_WORKER(mon) && FLAG_TIMES(mon)) || FLAG_LOAD(mon)) {
		/* write message */
		lpel_timing_t tnow;
		LpelTimingNow( &tnow);
		PrintNormTS( &tnow, mon->outfile);
	}

	if (FLAG_WORKER(mon) || FLAG_LOAD(mon)) {
		fprintf( mon->outfile, "%c%c", worker_end, end_entry);

		if (FLAG_LOAD(mon))
			printStatistic(mon);
	}


	if ( mon->outfile != NULL) {
		int ret;
		ret = fclose( mon->outfile);
		assert(ret == 0);
	}

	if ( mon->events.buffer != NULL) {
		free(mon->events.buffer);
	}

	free( mon);
}




static void MonCbWorkerWaitStart( mon_worker_t *mon)
{
	LpelTimingStart(&mon->wait_current);
	if (FLAG_LOAD(mon))
		mon->wait_cnt++;		// cheaper than without conditional check?
}


static void MonCbWorkerWaitStop(mon_worker_t *mon)
{
	if (FLAG_WORKER(mon) || FLAG_LOAD(mon)) {
		LpelTimingEnd(&mon->wait_current);
	}


	if (FLAG_WORKER(mon)) {
		if (FLAG_TIMES(mon)) {
			lpel_timing_t tnow;
			LpelTimingNow(&tnow);
			PrintNormTS(&tnow, mon->outfile);
		}


		// waiting time in second
		//		  fprintf(mon->outfile, "%c %lu.%09lu %c", worker_wait,
		//				(unsigned long) mon->wait_current.tv_sec, (unsigned long)mon->wait_current.tv_nsec, end_entry
		//		);


		/* waiting time in nanosecond */
		fprintf( mon->outfile, "%c", worker_wait);
		PrintTiming( &mon->wait_current, mon->outfile);
		fprintf( mon->outfile, "%c", end_entry);
	}

	if (FLAG_LOAD(mon))
		LpelTimingAdd(&mon->wait_time, &mon->wait_current);

}




static void MonCbTaskDestroy(mon_task_t *mt)
{
	assert( mt != NULL );
	free(mt);
}


/**
 * Called upon assigning a task to a worker.
 *
 * Sets the monitoring context of mt accordingly.
 */
static void MonCbTaskAssign(mon_task_t *mt, mon_worker_t *mw)
{
	assert( mt != NULL );
	assert( mt->mw == NULL );
	mt->mw = mw;
}

mon_task_t *SNetThreadingMonTaskCreate(unsigned long tid, const char *name)
{
	mon_task_t *mt = malloc( sizeof(mon_task_t) );

	/* zero out everything */
	memset(mt, 0, sizeof(mon_task_t));

	/* copy name and 0-terminate */
	if ( name != NULL ) {
		(void) strncpy(mt->name, name, MON_ENTNAME_MAXLEN);
		mt->name[MON_ENTNAME_MAXLEN-1] = '\0';
	}

	mt->mw = NULL;
	mt->tid = tid;
	mt->flags = mon_flags;

	mt->dirty_list = ST_DIRTY_END;

	if FLAG_TIMES(mt) {
		lpel_timing_t tnow;
		LpelTimingNow(&tnow);
		LpelTimingDiff(&mt->times.creat, &monitoring_begin, &tnow);
	}

	return mt;
}


static void MonCbTaskStart(mon_task_t *mt)
{
	assert( mt != NULL );
	if FLAG_TIMES(mt) {
		LpelTimingNow(&mt->times.start);
	}

	/* set blockon to any */
	mt->blockon = 'A';
}




static void MonCbTaskStop( mon_task_t *mt, lpel_taskstate_t state)
{
	if ( mt->mw==NULL) return;

	FILE *file = mt->mw->outfile;
	lpel_timing_t et;
	assert( mt != NULL );


	if FLAG_TIMES(mt) {
		LpelTimingNow(&mt->times.stop);
		PrintNormTS(&mt->times.stop, file);
	}

	/* print general info: status, id */

	if ( state==TASK_BLOCKED) {
		fprintf( file, "%c", mt->blockon);
	} else {
		fprintf( file, "%c", state);
	}
	fprintf( file, "%lu ", mt->tid);

	/* print times */
	if FLAG_TIMES(mt) {
		/* execution time */
		LpelTimingDiff(&et, &mt->times.start, &mt->times.stop);
		PrintTiming( &et , file);
		fprintf( file, " ");
		if ( state == TASK_ZOMBIE) {	// task finish
			PrintTiming( &mt->times.creat, file);
			fprintf(file, " ");
		}
	}

	/* print stream info */
	if FLAG_STREAMS(mt) {
		/* print (and reset) dirty list */
		PrintDirtyList(mt);
	}

	/* print user events */
	if FLAG_MESSAGE(mt) {
		PrintUsrEvt(mt);
	}

	fprintf( file, "%c", end_entry);

	//FIXME only for debugging purposes
	//	fflush( file);
}




static mon_stream_t *MonCbStreamOpen(mon_task_t *mt, unsigned int sid, char mode)
{
	if (!mt || !FLAG_STREAMS(mt)) return NULL;

	mon_stream_t *ms = malloc(sizeof(mon_stream_t));
	ms->sid = sid;
	ms->montask = mt;
	ms->mode = mode;
	ms->state = ST_OPENED;
	ms->counter = 0;
	ms->strevt_flags = 0;
	ms->dirty = NULL;

	MarkDirty(ms);

	return ms;
}

/**
 * @pre ms != NULL
 */
static void MonCbStreamClose(mon_stream_t *ms)
{
	assert( ms != NULL );
	ms->state = ST_CLOSED;
	MarkDirty(ms);
	/* do not free ms, as it will be kept until its monintoring
     information has been output via dirty list upon TaskStop() */
}



static void MonCbStreamReplace(mon_stream_t *ms, unsigned int new_sid)
{
	assert( ms != NULL );
	ms->state = ST_REPLACED;
	ms->sid = new_sid;
	MarkDirty(ms);
}

/**
 * @pre ms != NULL
 */
static void MonCbStreamReadPrepare(mon_stream_t *ms)
{
	(void) ms; /* NOT USED */
	return;
}

/**
 * @pre ms != NULL
 */
static void MonCbStreamReadFinish(mon_stream_t *ms, void *item)
{
	(void) item; /* NOT USED */
	assert( ms != NULL );

	ms->counter++;
	ms->strevt_flags |= ST_MOVED;
	MarkDirty(ms);
}

/**
 * @pre ms != NULL
 */
static void MonCbStreamWritePrepare(mon_stream_t *ms, void *item)
{
	(void) ms; /* NOT USED */
	(void) item; /* NOT USED */
	return;
}

/**
 * @pre ms != NULL
 */
static void MonCbStreamWriteFinish(mon_stream_t *ms)
{
	assert( ms != NULL );

	ms->counter++;
	ms->strevt_flags |= ST_MOVED;
	MarkDirty(ms);
}




/**
 * @pre ms != NULL
 */
static void MonCbStreamBlockon(mon_stream_t *ms)
{
	assert( ms != NULL );
	ms->strevt_flags |= ST_BLOCKON;
	MarkDirty(ms);

	/* track if blocked on reading or writing */
	switch(ms->mode) {
	case 'r': ms->montask->blockon = 'I'; break;
	case 'w': ms->montask->blockon = 'O'; break;
	default: assert(0);
	}
}


/**
 * @pre ms != NULL
 */
static void MonCbStreamWakeup(mon_stream_t *ms)
{
	assert( ms != NULL );
	ms->strevt_flags |= ST_WAKEUP;

	/* MarkDirty() not needed, as Moved()
	 * event is called anyway
	 */
	//MarkDirty(ms);
}




/*****************************************************************************
 * PUBLIC FUNCTIONS
 ****************************************************************************/



/**
 * Initialize the monitoring module
 *
 */
void SNetThreadingMonInit(lpel_monitoring_cb_t *cb, int node, int flag)
{

#ifdef USE_LOGGING
	mon_node = node;
	mon_flags = flag;

	/* register callbacks */

	if ((mon_flags & SNET_MON_WORKER) || (mon_flags & SNET_MON_TASK) || (mon_flags & SNET_MON_LOAD)) {
		cb->worker_create         = MonCbWorkerCreate;
		cb->worker_create_wrapper = MonCbWrapperCreate;
		cb->worker_destroy        = MonCbWorkerDestroy;
		cb->worker_waitstart      = MonCbWorkerWaitStart;
		cb->worker_waitstop       = MonCbWorkerWaitStop;
	}

	if (mon_flags & SNET_MON_TASK) {
		cb->task_destroy = MonCbTaskDestroy;
		cb->task_assign  = MonCbTaskAssign;
		cb->task_start   = MonCbTaskStart;
		cb->task_stop    = MonCbTaskStop;
	}

	if(mon_flags & SNET_MON_STREAM) {
		cb->stream_open         = MonCbStreamOpen;
		cb->stream_close        = MonCbStreamClose;
		cb->stream_replace      = MonCbStreamReplace;
		cb->stream_readprepare  = MonCbStreamReadPrepare;
		cb->stream_readfinish   = MonCbStreamReadFinish;
		cb->stream_writeprepare = MonCbStreamWritePrepare;
		cb->stream_writefinish  = MonCbStreamWriteFinish;
		cb->stream_blockon      = MonCbStreamBlockon;
		cb->stream_wakeup       = MonCbStreamWakeup;
	}
	/* initialize timing */
	LpelTimingNow(&monitoring_begin);
#endif
}


/**
 * Cleanup the monitoring module
 */
void SNetThreadingMonCleanup(void)
{
	/* NOP */
}


void SNetThreadingMonEvent(mon_task_t *mt, snet_moninfo_t *moninfo)
{
	assert(mt != NULL);
	mon_worker_t *mw = mt->mw;


	if (FLAG_MESSAGE(mt) && mw) {
		/* grow events buffer if needed */
		if (mw->events.cnt == mw->events.size) {
			/* grow */
			mon_usrevt_t *newbuf = malloc(
					(mw->events.size + MON_USREVT_BUFSIZE_DELTA ) * sizeof(mon_usrevt_t)
			);
			if (mw->events.buffer != NULL) {
				(void) memcpy(newbuf, mw->events.buffer, mw->events.size * sizeof(mon_usrevt_t));
				free(mw->events.buffer);
			}
			mw->events.size += MON_USREVT_BUFSIZE_DELTA;
			mw->events.buffer = newbuf;
		}
		/* get a pointer to a usrevt slot */
		mon_usrevt_t *me = &mw->events.buffer[mw->events.cnt];
		/* set values */
		if FLAG_TIMES(mt) LpelTimingNow(&me->ts);
		me->moninfo = moninfo;
		me->in_cnt = mt->last_in_cnt;
		me->out_cnt = mt->last_out_cnt;

		mw->events.cnt++;
	}
}



