/*
 * Provide utility functions which are highly dependent
 * on the used runtime (Pthread/LPEL/Front/...).
 * These are typically used on startup.
 *
 * This is the version for the Front RTS.
 */

#include "distribution.h"
#include "stream.h"
#include "networkinterface.h"
#include <stdlib.h>

static const char snet_front_help_text[] =
"Usage: <executable name> [options...]\n"
"The Front runtime system for S-Net supports the following options:\n"
"\t-c <spec>\tSet concurrent box invocations according to <spec>.\n"
"\t-d \t\tEnable debugging output.\n"
"\t-g \t\tDisable garbage collection of network nodes (debugging).\n"
"\t-h \t\tDisplay this help text.\n"
"\t-i <filename>\tRead input records from file <filename>.\n"
"\t-I <port>\tInput records from socket at portnumber <port>.\n"
"\t-o <filename>\tOutput to the file <filename>.\n"
"\t-O <addr:port>\tOutput to destination host <addr> and port <port>.\n"
"\t-r \t\tEnable dynamic control of the number of worker threads.\n"
"\t-rs [<host:port>] Use the distributed resource management service.\n"
"\t-s <size(K|M)>\tSet thread stack size to <size> K or M.\n"
"\t-t <depth>\tEnable function call tracing with call stack <depth>.\n"
"\t-T <count>\tLimit the number of thieves to <count>.\n"
"\t-v \t\tEnable informational messages.\n"
"\t-V \t\tGive the runtime system version and contact information.\n"
"\t-w <count>\tSet number of workers to <count>.\n"
"\t-x <offset>\tAllow for an unconditional <offset> number of input records.\n"
"\t-y <factor>\tNever input more than <offset> + <factor> * #output records.\n"
"\t-z \t\tDisable the zipper (This is only useful for debugging).\n"
"\t-FD\t\tMake the single-backslash feedback combinator deterministic.\n"
;


void SNetRuntimeHelpText(void)
{
  puts(snet_front_help_text);
}

void SNetRuntimeStartWait(snet_stream_t *in, snet_info_t *info, snet_stream_t *out)
{
  void SNetNodeRun(snet_stream_t *in, snet_info_t *info, snet_stream_t *out);
  SNetNodeRun(in, info, out);
}

snet_runtime_t SNetRuntimeGet(void)
{
  return Front;
}

