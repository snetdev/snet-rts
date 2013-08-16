/*
 * Provide utility functions which are highly dependent
 * on the used runtime (Pthread/LPEL/Front/...).
 * These are typically used on startup.
 *
 * This is the version for the Threaded-Entity RTS (Pthread/LPEL).
 */

#include <assert.h>
#include "distribution.h"
#include "stream.h"
#include "networkinterface.h"

void SNetRuntimeHelpText(void)
{
  printf("usage: <executable name> [options]\n");
  printf("\nOptions:\n");
  printf("\t-i <filename>\t\tInput from file.\n");
  printf("\t-I <port>\t\tInput from socket.\n");
  printf("\t-h \t\t\tDisplay this help text.\n");
  printf("\t-o <filename>\t\tOutput to file.\n");
  printf("\t-O <address:port>\tOutput to socket.\n");
  printf("\t-m <mon_level>\t\tSet monitoring level (LPEL only).\n");
  printf("\t-w <workers>\t\tSet number of workers (LPEL) / visible cores (PThread).\n");
  printf("\n");
}

void SNetRuntimeStartWait(snet_stream_t *in, snet_info_t *info, snet_stream_t *out)
{
  SNetDistribWaitExit(info);
}

/* Copy the stack of detref references from one record to another. */
void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *old_rec)
{
  /* Only needed for the Front-RTS. */
}

snet_runtime_t SNetRuntimeGet(void)
{
  return Streams;
}

void SNetRecDetrefStackSerialise(snet_record_t *rec, void *buf)
{ /* Only needed for the Front-RTS. */ }

void SNetRecDetrefRecSerialise(snet_record_t *rec, void *buf)
{ /* Only possible in the Front-RTS */ assert(false); }

void SNetRecDetrefStackDeserialise(snet_record_t *rec, void *buf)
{ /* Only needed for the Front-RTS. */ }

snet_record_t* SNetRecDetrefRecDeserialise(void *buf)
{ /* Only possible in the Front-RTS */ assert(false); }

