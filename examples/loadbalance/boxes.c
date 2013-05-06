#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <C4SNet.h>

typedef struct { int x, y, xdim, ydim, i;    } state_t;
typedef struct { int x, y, xdim, r, g, b, i; } work_t;
typedef struct { char *map; int l, c, xdim;  } collect_t;

static int ncores() { return (int)sysconf(_SC_NPROCESSORS_ONLN); }

void* init(void *hnd, int xdim, int ydim, int nodes, int states)
{
  c4snet_data_t *c4state, *c4coll;
  void          *vptr;
  state_t       *state;
  collect_t     *collect;
  char          *map;
  int            i, chunk;
  int fd = open("out.pnm", O_RDWR|O_TRUNC|O_CREAT, 0666);

  lseek(fd, 31 + 3 * xdim * ydim, SEEK_SET);
  (void) write(fd, "", 1);
  lseek(fd, 0, SEEK_SET);
  map = mmap(0, 32 + 3 * xdim * ydim,
             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0L);
  close(fd);
  sprintf(map, "P6 %d %d 255\n", xdim, ydim);

  states = MIN(ydim, MAX(states, 1));
  chunk = (ydim + (states - 1)) / states;
  for (i = 0; i * chunk < ydim; ++i) {
    int n = sizeof(state_t)/sizeof(int);
    c4state = C4SNetAlloc(CTYPE_int, n, &vptr);
    state = vptr;
    state->x = 0;
    state->y = i * chunk;
    state->xdim = xdim;
    state->ydim = MIN(ydim, (i + 1) * chunk);
    state->i = i;
    C4SNetOut( hnd, 1, c4state);

    n = sizeof(collect_t)/sizeof(int);
    c4coll = C4SNetAlloc(CTYPE_int, n, &vptr);
    collect = vptr;
    collect->map = map + strlen(map);
    collect->l = ((i + 1) * chunk >= ydim);
    collect->c = xdim * (state->ydim - i * chunk);
    collect->xdim = xdim;
    C4SNetOut( hnd, 2, c4coll, i);
  }

  for (i = 0; i < nodes; ++i) {
    C4SNetOut( hnd, 3, NULL, i);
  }
  return hnd;
}

void* estimator(void *hnd, c4snet_data_t *probe, int node)
{
  int work = (node == 0) ? (ncores() - 1) : 2*ncores();
  do {
    C4SNetOut( hnd, 1, node);
  } while (--work > 0);
  return hnd;
}

void* inbox(void *hnd, c4snet_data_t *c4state, int node)
{
  state_t *state = C4SNetGetData(c4state);
  if (state->x < state->xdim) {
    void *vptr;
    int n = sizeof(work_t)/sizeof(int);
    c4snet_data_t *c4work = C4SNetAlloc(CTYPE_int, n, &vptr);
    work_t *work = vptr;
    work->x = state->x;
    work->y = state->y;
    work->xdim = state->xdim;
    work->i = state->i;
    C4SNetOut( hnd, 2, c4work, node);
    if (++state->x >= state->xdim && ++state->y < state->ydim) {
      state->x = 0;
    }
  }
  if (state->x < state->xdim) {
    C4SNetOut( hnd, 1, c4state);
  } else {
    C4SNetFree(c4state);
  }
  return hnd;
}

void* worker(void *hnd, c4snet_data_t *c4work, int node)
{
  work_t *work = C4SNetGetData(c4work);
  work->r = (int)floor(127.5 * (1.0 + sin(work->x / 100.0)));
  work->g = (int)floor(127.5 * (1.0 + cos(work->y / 100.0)));
  work->b = (int)floor(255.0 * work->x / work->xdim);
  C4SNetOut( hnd, 1, c4work, work->i);
  C4SNetOut( hnd, 2, node);
  return hnd;
}

void* merge(void *hnd, c4snet_data_t *c4coll, c4snet_data_t *c4result)
{
  work_t *work = C4SNetGetData(c4result);
  collect_t *coll = C4SNetGetData(c4coll);
  int index = 3*(work->x + work->y * work->xdim);
  unsigned char *pix = (unsigned char *) &coll->map[index];
  pix[0] = work->r; pix[1] = work->g; pix[2] = work->b;
  C4SNetFree(c4result);
  if (--coll->c > 0) {
    C4SNetOut( hnd, 1, c4coll);
  } else {
    if (coll->l) {
      C4SNetOut( hnd, 2, C4SNetCreate(CTYPE_char, 5, "Done."));
    }
    C4SNetFree(c4coll);
  }
  return hnd;
}

