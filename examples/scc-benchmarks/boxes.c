#define _BSD_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "memfun.h"
#include "C4SNet.h"

extern int node_location;
static inline volatile unsigned long long rdtsc()
{
  register long long TSC __asm__("eax");
  __asm__ volatile (".byte 15, 49" : : : "eax", "edx");
  return TSC;
}

void *t_start(void *hnd)
{
  time_t start;
  c4snet_data_t *snet_time;

  time(&start);
  printf("t_start: time = %ld\n", start);

  snet_time = C4SNetDataCreate( CTYPE_long, &start);

  C4SNetOut( hnd, 1, snet_time);

  return hnd;
}

void *t_end( void *hnd, c4snet_data_t *snet_time)
{
  time_t start, end;
  c4snet_data_t *snet_res;

  time(&end);
  start = *(long*)C4SNetDataGetData(snet_time);
  printf("t_end: time = %lu\n", end);
  printf("runtime: time = %lu\n", end - start);

  C4SNetOut(hnd, 1);
  C4SNetDataFree(snet_time);

  return hnd;
}

void *split(void *hnd, int nodes, int tasks, int dataSize)
{
  void *data;
  c4snet_data_t *snet_data;

  for (int i = 0; i < tasks; i++) {
    snet_data = C4SNetDataAllocCreateArray(CTYPE_char, dataSize, &data);
    C4SNetOut(hnd, 1, tasks, i, snet_data);
    if (i < nodes) C4SNetOut(hnd, 2, i);
  }

  return hnd;
}

void *dummy(void *hnd, int sleep, c4snet_data_t *old)
{
  void *data;
  int size = C4SNetDataGetArraySize(old);
  c4snet_data_t *snet_data = C4SNetDataAllocCreateArray(CTYPE_char, size, &data);
  usleep(100000 * sleep);
  C4SNetOut(hnd, 1, snet_data);
  C4SNetDataFree(old);
  return hnd;
}

void *rename_field(void *hnd, int count, c4snet_data_t *data)
{
  C4SNetOut(hnd, 1, count, data);
  return hnd;
}

void *merge(void *hnd, c4snet_data_t *data, c4snet_data_t *DATA)
{
  C4SNetOut(hnd, 1, DATA);
  C4SNetDataFree(data);
  return hnd;
}
