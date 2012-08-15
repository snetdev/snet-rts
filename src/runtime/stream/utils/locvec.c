
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "locvec.h"
#include "memfun.h"

#define LOCVEC_INFO_TAG 493


#define LOCVEC_CAPACITY_DELTA 1



typedef enum {
  LOC_UNDEFINED = '?',
  LOC_SERIAL = 'S',
  LOC_PARALLEL = 'P',
  LOC_SPLIT = 'I',
  LOC_STAR = 'R',
  LOC_FEEDBACK = 'F',
  /*
  LOC_BOX = 'B',
  LOC_FILTER = 'L',
  LOC_SYNC = 'Y'
  */
} snet_loctype_t;

typedef struct {
  snet_loctype_t type;
  int num;
} snet_locitem_t;

struct snet_locvec_t {
  int size;
  int capacity;
  snet_locitem_t *arr;
};


static void SNetLocvecAppend(snet_locvec_t *vec, snet_loctype_t type, int num);
static void SNetLocvecPop(snet_locvec_t *vec);
static snet_loctype_t SNetLocvecToptype(snet_locvec_t *vec);
static void SNetLocvecTopinc(snet_locvec_t *vec);
static void SNetLocvecTopdec(snet_locvec_t *vec);
int SNetLocvecTopval(snet_locvec_t *vec);



/*****************************************************************************
 * PUBLIC FUNCTIONS
 ****************************************************************************/


snet_locvec_t *SNetLocvecCreate(void)
{
  snet_locvec_t *vec = SNetMemAlloc(sizeof(snet_locvec_t));

  vec->size = 0;
  vec->capacity = LOCVEC_CAPACITY_DELTA;
  vec->arr = SNetMemAlloc(LOCVEC_CAPACITY_DELTA * sizeof(snet_locitem_t));
  return vec;
}

void SNetLocvecDestroy(snet_locvec_t *vec)
{
  SNetMemFree( vec->arr );
  SNetMemFree( vec );
}

snet_locvec_t *SNetLocvecCopy(snet_locvec_t *vec)
{
  snet_locvec_t *newvec = SNetMemAlloc(sizeof(snet_locvec_t));
  /* shallow copy of fields */
  *newvec = *vec;
  /* copy arr */
  newvec->arr = SNetMemAlloc(newvec->capacity * sizeof(snet_locitem_t));
  (void) memcpy(newvec->arr, vec->arr, newvec->size * sizeof(snet_locitem_t));
  return newvec;
}





bool SNetLocvecEqual(snet_locvec_t *u, snet_locvec_t *v)
{
  int i;

  if (u==v) return true;
  if (u->size != v->size) return false;

  for (i=0; i<u->size; i++) {
    if (u->arr[i].type != v->arr[i].type ||
        u->arr[i].num  != v->arr[i].num ) {
      return false;
    }
  }
  return true;
}

bool SNetLocvecEqualParent(snet_locvec_t *u, snet_locvec_t *v)
{
  int i;

  if (u==v) return true;
  if (u->size != v->size) return false;

  for (i=0; i<u->size-1; i++) {
    if (u->arr[i].type != v->arr[i].type ||
        u->arr[i].num  != v->arr[i].num ) {
      return false;
    }
  }

  i = u->size-1;
  if ( (u->arr[i].type) != (v->arr[i].type) ) {
    return false;
  }
  assert((u->arr[i].num) < (v->arr[i].num));
  /*
  if( ((u->arr[i].num) != -1) &&
      ((u->arr[i].num) < (v->arr[i].num)) ) {
    return false;
  }
  */
  return true;
}






/* for serial combinator */

bool SNetLocvecSerialEnter(snet_locvec_t *vec)
{
  if (SNetLocvecToptype(vec) != LOC_SERIAL) {
    SNetLocvecAppend(vec, LOC_SERIAL, 1);
    return true;
  }
  return false;
}

void SNetLocvecSerialNext(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_SERIAL );
  SNetLocvecTopinc(vec);
}


void SNetLocvecSerialLeave(snet_locvec_t *vec, bool enter)
{
  assert( SNetLocvecToptype(vec) == LOC_SERIAL );
  if (enter) {
    SNetLocvecPop(vec);
  } else {
  //TODO clear leaf
  }
}

/* for parallel combinator */
void SNetLocvecParallelEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_PARALLEL, -1);
}


void SNetLocvecParallelNext(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_PARALLEL );
  SNetLocvecTopinc(vec);
}


void SNetLocvecParallelLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_PARALLEL );
  SNetLocvecPop(vec);
}

void SNetLocvecParallelReset(snet_locvec_t *vec)
{
  SNetLocvecParallelLeave(vec);
  SNetLocvecParallelEnter(vec);
}


/* for split combinator */
void SNetLocvecSplitEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_SPLIT, -1);
}

void SNetLocvecSplitLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_SPLIT );
  SNetLocvecPop(vec);
}

snet_locvec_t *SNetLocvecSplitSpawn(snet_locvec_t *vec, int i)
{
  assert( SNetLocvecToptype(vec) == LOC_SPLIT );
  //TODO assert vec is disp
  snet_locvec_t *copy = SNetLocvecCopy(vec);
  SNetLocvecPop(copy);
  SNetLocvecAppend(copy, LOC_SPLIT, i);
  return copy;
}



/* for star combinator */
bool SNetLocvecStarWithin(snet_locvec_t *vec)
{
  return ( SNetLocvecToptype(vec) == LOC_STAR );
}

void SNetLocvecStarEnter(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) != LOC_STAR );

  SNetLocvecAppend(vec, LOC_STAR, -1);
}

snet_locvec_t *SNetLocvecStarSpawn(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_STAR );
  SNetLocvecTopinc(vec);
  return vec;
}

snet_locvec_t *SNetLocvecStarSpawnRet(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_STAR );
  SNetLocvecTopdec(vec);
  return vec;
}

void SNetLocvecStarLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_STAR );
  assert( SNetLocvecTopval(vec) == -1);

  SNetLocvecPop(vec);
}



/* for feedback combinator */
void SNetLocvecFeedbackEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_FEEDBACK, -1);

}


void SNetLocvecFeedbackLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_FEEDBACK );
  SNetLocvecPop(vec);
}


/**
 * INFO STRUCTURE SETTING/RETRIEVAL
 */

snet_locvec_t *SNetLocvecGet(snet_info_t *info)
{
  return (snet_locvec_t*) SNetInfoGetTag(info, LOCVEC_INFO_TAG);
}


void SNetLocvecSet(snet_info_t *info, snet_locvec_t *vec)
{
  SNetInfoSetTag(info, LOCVEC_INFO_TAG, (uintptr_t) vec, NULL);
}






inline static int max(int x, int y) { return x > y ? x : y; }

int SNetLocvecPrintSize(snet_locvec_t *vec)
{
  int cnt = 0;

  for (int i = 0; i < vec->size; i++) {
    cnt += 2;
    if (vec->arr[i].num >= 0) {
      cnt += max(1, ceil(log10(vec->arr[i].num + 1)));
    }
  }

  return cnt;
}

void SNetLocvecPrint(char *sbuf, snet_locvec_t *vec)
{
  int val;
  int offset = 0;

  for (int i = 0; i < vec->size; i++) {
    snet_locitem_t *item = &vec->arr[i];
    if (item->num >= 0) {
      val = sprintf(sbuf + offset, ":%c%d", (char)item->type, item->num);
      assert(val >= 0);
      offset += val;
    } else {
      val = sprintf(sbuf + offset, ":%c", (char)item->type);
      assert(val >= 0);
      offset += val;
    }
  }
}


/******************************************************************************
 * Static helper functions
 *****************************************************************************/

static void SNetLocvecAppend(snet_locvec_t *vec, snet_loctype_t type, int num)
{
  if (vec->size == vec->capacity) {
    /* grow */
    snet_locitem_t *newarr = SNetMemAlloc(
        (vec->capacity + LOCVEC_CAPACITY_DELTA) * sizeof(snet_locitem_t)
        );
    vec->capacity += LOCVEC_CAPACITY_DELTA;
    (void) memcpy(newarr, vec->arr, vec->size * sizeof(snet_locitem_t));
    SNetMemFree(vec->arr);
    vec->arr = newarr;
  }

  snet_locitem_t *item = &vec->arr[vec->size];
  item->type = type;
  item->num = num;

  vec->size++;
}


static void SNetLocvecPop(snet_locvec_t *vec)
{
  vec->size--;
}


static snet_loctype_t SNetLocvecToptype(snet_locvec_t *vec)
{
  if (vec->size > 0) return vec->arr[vec->size-1].type;
  else return LOC_UNDEFINED;
}

int SNetLocvecTopval(snet_locvec_t *vec)
{
  return vec->arr[vec->size-1].num;
}


static void SNetLocvecTopinc(snet_locvec_t *vec)
{
  snet_locitem_t *item = &vec->arr[vec->size-1];
  item->num++;
}

static void SNetLocvecTopdec(snet_locvec_t *vec)
{
  snet_locitem_t *item = &vec->arr[vec->size-1];
  item->num--;
}

