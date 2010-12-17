#ifndef _MONITORING_H_
#define _MONITORING_H_


#define MONITORING_ENABLE



#ifdef MONITORING_ENABLE

#include <stdio.h>

//#define MON_DO_FLUSH

//#define MON_MAX_EVENTS  10

#define MONITORING_TIMES        (1<<0)
#define MONITORING_STREAMINFO   (1<<1)

#define MONITORING_NONE           ( 0)
#define MONITORING_ALL            (-1)




typedef struct {
  FILE *outfile;
  int   flags;
//  int   num_events;
//  char  events[MON_MAX_EVENTS][20];
} monitoring_t;

struct lpelthread;
struct schedctx;

void MonInit( struct lpelthread *env, int flags);
void MonCleanup( struct lpelthread *env);
void MonDebug( struct lpelthread *env, const char *fmt, ...);

struct task;

//void MonTaskEvent( struct task *t, const char *fmt, ...);
void MonTaskPrint( monitoring_t *mon, struct schedctx *sc, struct task *t);



#endif /* MONITORING_ENABLE */

#endif /* _MONITORING_H_ */
