#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "C4SNet.h"

extern int node_location;

void* tell(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell0(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell1(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell2(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell3(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell4(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* tell5(void *hnd, int I)
{
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d\n", __func__, node_location, I);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, I);
  return hnd;
}

void* incr(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr0(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr1(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr2(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr3(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr4(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

void* incr5(void *hnd, int I)
{
  int Io = I + 1;
  fflush(stdout);
  usleep(10000);
  printf("%s @ %d: I %d -> Io %d\n", __func__, node_location, I, Io);
  fflush(stdout);
  usleep(10000);
  C4SNetOut(hnd, 1, Io);
  return hnd;
}

