#ifndef _TRACE_H
#define _TRACE_H

/* Choose whether to enable or disable function call tracing */
#if !defined(ENABLE_TRACE) && !defined(NTRACE)
#   define ENABLE_TRACE    1
#endif

/* Only trace when ENABLE_TRACE is true */
#if ENABLE_TRACE
    extern void (*SNetTraceFunc)(const char *func);
#   define trace(x)     (*SNetTraceFunc)(x)
#else
#   define trace(x)     /*nothing*/
#endif



/* Show a function name and a small stacktrace. */
void SNetTraceCalls(const char *func);

/* Trace only the function name. */
void SNetTraceSimple(const char *func);

/* Trace nothing. */
void SNetTraceEmpty(const char *func);

/* Set tracing: -1 disables, >= 0 sets call stack tracing depth. */
void SNetEnableTracing(int trace_level);

/* Force tracing of a function call at a specific depth. */
void SNetTrace(const char *func, int trace_level);

#endif
