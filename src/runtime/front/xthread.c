#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include "node.h"
#include "reference.h"
#include "networkinterface.h"

#define EQ(s1,s2)       !strcmp(s1,s2)
#define EQN(s1,s2,n)    !strncmp(s1,s2,n)

static pthread_key_t    thread_self_key;
static int              num_workers;
static int              num_thieves;
static const char      *program_name;
static const char      *opt_concurrency;
static bool             opt_debug;
static bool             opt_debug_df;
static bool             opt_debug_gc;
static bool             opt_debug_rs;
static bool             opt_debug_sl;
static bool             opt_debug_tl;
static bool             opt_debug_ws;
static bool             opt_feedback_deterministic;
static bool             opt_garbage_collection;
static double           opt_input_factor;
static double           opt_input_offset;
static bool             opt_input_throttle;
static bool             opt_resource;
static const char      *opt_resource_server;
static size_t           opt_thread_stack_size;
static bool             opt_verbose;
static bool             opt_zipper;

/* Total number of cores in system, whether currently online or not. */
int SNetGetMaxProcs(void)
{
  return (int) sysconf(_SC_NPROCESSORS_CONF);
}

/* The number of cores in the system which are currently operational. */
int SNetGetNumProcs(void)
{
  return (int) sysconf(_SC_NPROCESSORS_ONLN);
}

/* How many workers? */
int SNetThreadingWorkers(void)
{
  return num_workers;
}

/* Limit the number of actively stealing thieves, if non-zero. */
int SNetThreadingThieves(void)
{
  return num_thieves;
}

/* How many distributed computing threads? */
int SNetThreadedManagers(void)
{
  return SNetDistribIsDistributed() ? 1 : 0;
}

/* Name of this program. */
const char* SNetGetProgramName(void)
{
  return program_name;
}

/* What kind of garbage collection to use? */
bool SNetGarbageCollection(void)
{
  return opt_garbage_collection;
}

/* Whether to be verbose */
bool SNetVerbose(void)
{
  return opt_verbose;
}

/* Whether to enable debugging information */
bool SNetDebug(void) { return opt_debug; }

/* Whether debugging of distributed front is enabled */
bool SNetDebugDF(void) { return opt_debug_df; }

/* Whether debugging of garbage collection is enabled */
bool SNetDebugGC(void) { return opt_debug_gc; }

/* Whether debugging of resource management service is enabled */
bool SNetDebugRS(void) { return opt_debug_rs; }

/* Whether debugging of the streams layer is enabled */
bool SNetDebugSL(void) { return opt_debug_sl; }

/* Whether debugging of the threading layer is enabled */
bool SNetDebugTL(void) { return opt_debug_tl; }

/* Whether debugging of work stealing is enabled */
bool SNetDebugWS(void) { return opt_debug_ws; }

/* Whether to use a deterministic feedback */
bool SNetFeedbackDeterministic(void)
{
  return opt_feedback_deterministic;
}

/* Whether to use dynamic resource management. */
bool SNetOptResource(void)
{
  return opt_resource;
}

/* Whether and how to connect to the resource management service. */
const char* SNetOptResourceServer(void)
{
  return opt_resource_server;
}

/* Whether to use optimized sync-star. */
bool SNetZipperEnabled(void)
{
  return opt_zipper;
}

/* The stack size for worker threads in bytes */
size_t SNetThreadStackSize(void)
{
  return opt_thread_stack_size;
}

/* Whether to apply an input throttle. */
bool SNetInputThrottle(void)
{
  return opt_input_throttle;
}

/* The number of unconditional input records. */
double SNetInputOffset(void)
{
  return opt_input_offset;
}

/* The rate at which input can increase depending on output. */
double SNetInputFactor(void)
{
  return opt_input_factor;
}

/* Extract the box concurrency specification for a given box name.
 * The default concurrency specification can be given as a number,
 * which is optionally followed by a capital 'D' for determinism.
 *      16D
 * Or as a number followed by a list of box names to which it applies:
 *      3:foo,bar
 * Or as a combination of these separated by spaces:
 *      16D 3:foo,bar 4D:mit 1:out
 * A default concurrency applies only if no box specific one exists.
 */
int SNetGetBoxConcurrency(const char *box, bool *is_det)
{
  int   default_conc = 1;
  bool  default_det = false;
  int   conc = -1;
  char *line, *line_save, *word, *word_save;

  if (opt_concurrency) {
    char *copy = strdup(opt_concurrency);
    if ((line = strtok_r(copy, " \t", &line_save)) != NULL) {
      do {
        if ((word = strtok_r(line, "=:", &word_save)) != NULL) {
          int word_count = 0;
          int num = atoi(word);
          bool det = (strchr(word, 'D') != NULL);
          while ((word = strtok_r(NULL, ",;", &word_save)) != NULL) {
            ++word_count;
            if (!strcmp(word, box)) {
              conc = num;
              *is_det = det;
              goto ok;
            }
          }
          if (word_count == 0) {
            default_conc = num;
            default_det = det;
          }
        }
      } while ((line = strtok_r(NULL, " \t", &line_save)) != NULL);
    }
ok: free(copy);
  }
  if (conc == -1) {
    conc = default_conc;
    *is_det = default_det;
  }
  if (conc <= 0) {
    SNetUtilDebugFatal("%s: Invalid box concurrency specification", __func__);
  }
  return conc;
}

/* Convert a string number which may be suffixed with K or M to bytes. */
static size_t GetSize(const char *str)
{
  double size = 0;
  char ch = '\0';
  int n = sscanf(str, "%lf%c", &size, &ch);
  if (n == 2) {
    switch (ch) {
      case 'k': case 'K': size *= 1024; break;
      case 'm': case 'M': size *= 1024*1024; break;
      default: size = 0; break;
    }
  }
  if (n < 1 || size < 0) {
    size = 0;
  }
  return (size_t) (size + 0.5);
}

/* Process command line options. */
int SNetThreadingInit(int argc, char**argv)
{
  int   i;

  trace(__func__);

  /* default options */
  num_workers = SNetGetNumProcs();
  opt_garbage_collection = true;
  opt_zipper = true;
  program_name = basename(argv[0]);

  for (i = 0; i < argc; ++i) {
    if (argv[i][0] != '-') {
    }
    else if (EQ(argv[i], "-c") && ++i < argc) {
      opt_concurrency = argv[i];
    }
    else if (EQN(argv[i], "-d", 2)) {
      if (EQ(argv[i], "-d")) {
        opt_debug = true;
      } else {
        if (strstr(argv[i], "df")) { opt_debug_df = true; }
        if (strstr(argv[i], "gc")) { opt_debug_gc = true; }
        if (strstr(argv[i], "rs")) { opt_debug_rs = true; }
        if (strstr(argv[i], "sl")) { opt_debug_sl = true; }
        if (strstr(argv[i], "tl")) { opt_debug_tl = true; }
        if (strstr(argv[i], "ws")) { opt_debug_ws = true; }
      }
    }
    else if (EQ(argv[i], "-g")) {
      opt_garbage_collection = false;
    }
    else if (EQ(argv[i], "-r")) {
      opt_resource = true;
    }
    else if (EQ(argv[i], "-rs")) {
      opt_resource = true;
      if (i + 1 < argc && argv[i+1][0] != '-') {
        opt_resource_server = argv[++i];
      } else {
        static const char default_resource_server[] = "localhost:56389";
        opt_resource_server = default_resource_server;
      }
    }
    else if (EQ(argv[i], "-s") && ++i < argc) {
      if ((opt_thread_stack_size = GetSize(argv[i])) < PTHREAD_STACK_MIN) {
        SNetUtilDebugFatal("[%s]: Invalid thread stack size %s (l.t. %d).",
                           __func__, argv[i], PTHREAD_STACK_MIN);
      }
    }
    else if (EQ(argv[i], "-t") && ++i < argc) {
      SNetEnableTracing(atoi(argv[i]));
    }
    else if (EQ(argv[i], "-v")) {
      opt_verbose = true;
    }
    else if (EQ(argv[i], "-V")) {
      SNetRuntimeVersion();
    }
    else if (EQ(argv[i], "-T") && ++i < argc) {
      if ((num_thieves = atoi(argv[i])) <= 0) {
        SNetUtilDebugFatal("[%s]: Invalid number of thieves %d.",
                           __func__, num_thieves);
      }
    }
    else if (EQ(argv[i], "-w") && ++i < argc) {
      if ((num_workers = atoi(argv[i])) <= 0) {
        SNetUtilDebugFatal("[%s]: Invalid number of workers %d.",
                           __func__, num_workers);
      }
    }
    else if (EQ(argv[i], "-x") && ++i < argc) {
      if ((opt_input_offset = atof(argv[i])) < 1.0) {
        SNetUtilDebugFatal("[%s]: Invalid input offset %f.",
                           __func__, opt_input_offset);
      } else {
        opt_input_throttle = true;
      }
    }
    else if (EQ(argv[i], "-y") && ++i < argc) {
      if ((opt_input_factor = atof(argv[i])) <= 0) {
        SNetUtilDebugFatal("[%s]: Invalid input factor %f.",
                           __func__, opt_input_factor);
      } else if (opt_input_offset == 0.0) {
        opt_input_offset = 1.0;
        opt_input_throttle = true;
      }
    }
    else if (EQ(argv[i], "-FD")) {
      opt_feedback_deterministic = true;
    }
    else if (EQ(argv[i], "-FN")) {
      opt_feedback_deterministic = false;
    }
    else if (EQ(argv[i], "-z")) {
      opt_zipper = false;
    }
  }

  if (opt_verbose) {
    printf("W=%d,GC=%s,Z=%s,R=%s,RS=%s.\n",
           num_workers,
           opt_garbage_collection ? "true" : "false",
           opt_zipper ? "true" : "false",
           opt_resource ? "true" : "false",
           opt_resource_server ? "true" : "false"
           );
  }

  pthread_key_create(&thread_self_key, NULL);

  return 0;
}

/* Create a thread to instantiate a worker */
void SNetThreadCreate(void *(*func)(void *), worker_t *worker, int proc)
{
  pthread_t             thread;
  pthread_attr_t        attr;

  trace(__func__);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (opt_thread_stack_size >= PTHREAD_STACK_MIN) {
    pthread_attr_setstacksize(&attr, opt_thread_stack_size);
  }

  if (pthread_create(&thread, &attr, func, worker)) {
    SNetUtilDebugFatal("[%s]: Failed to create a new thread.", __func__);
  }
  if (SNetDebugTL() && SNetVerbose() && !SNetOptResource()) {
    printf("created thread %lu for worker %d\n",
           *(unsigned long *)thread, worker->id);
  }

  pthread_attr_destroy(&attr);
}

void SNetThreadSetSelf(worker_t *self)
{
  pthread_setspecific(thread_self_key, self);
}

worker_t *SNetThreadGetSelf(void)
{
  return pthread_getspecific(thread_self_key);
}

int SNetThreadingStop(void)
{
  trace(__func__);

  SNetNodeStop();

  return 0;
}

int SNetThreadingCleanup(void)
{
  trace(__func__);

  pthread_key_delete(thread_self_key);

  SNetReferenceDestroy();

  SNetNodeCleanup();

  return 0;
}

