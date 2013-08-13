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

static const char snet_front_help_text[] =
"Usage: <executable name> [options]\n"
"Options:\n"
"\t-c <spec>\tSet concurrent box invocations according to <spec> (Front).\n"
"\t-i <filename>\tInput from file.\n"
"\t-I <port>\tInput from socket.\n"
"\t-g \t\tDisable garbage collection of network nodes (Front only).\n"
"\t-h \t\tDisplay this help text.\n"
"\t-m <mon_level>\tSet monitoring level (LPEL only).\n"
"\t-o <filename>\tOutput to file.\n"
"\t-O <addr:port>\tOutput to socket.\n"
"\t-s <size(K|M)>\tSet thread stack size to <size> K or M (Front only).\n"
"\t-t <depth>\tEnable function call tracing with call stack <depth> (Front).\n"
"\t-T <count>\tLimit number of thieves to <count> (Front).\n"
"\t-v \t\tEnable informational messages.\n"
"\t-w <count>\tSet number of workers (LPEL and Front) /\n"
            "\t\t\tNumber of visible cores (PThread)\n"
"\t-x <offset>\tAllow for an unconditional <offset> number of input records.\n"
"\t-y <factor>\tNever input more than <offset> + <factor> * #output records.\n"
"\t-z \t\tDisable the zipper (Front debugging).\n"
"\t-FD\t\tMake the feedback combinator deterministic (Front).\n"
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

