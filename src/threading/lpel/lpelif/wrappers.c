#include <assert.h>

#include "lpel.h"
#include "threading.h"
#include "memfun.h"

typedef struct {
  snet_locvec_t *source;
  struct {
    void (*func) (void *);
    void *arg;
  } callback_read;
} usrdata_t;

static usrdata_t *CheckCreateUsrdata(lpel_stream_t * ls)
{
  usrdata_t *dat = LpelStreamGetUsrData(ls);
  if (dat == NULL) {
    dat = SNetMemAlloc(sizeof(usrdata_t));
    dat->source = NULL;
    dat->callback_read.func = NULL;
    dat->callback_read.arg = NULL;
    LpelStreamSetUsrData(ls, dat);
  }
  return dat;
}

static void DestroyUsrdata(lpel_stream_t * ls)
{
  usrdata_t *dat = LpelStreamGetUsrData(ls);
  if (dat != NULL) {
    if (dat->source != NULL) {
      SNetLocvecDestroy(dat->source);
    }
    /* nothing to do for callback */
    /* free the usrdat container itself */
    SNetMemFree(dat);
    LpelStreamSetUsrData(ls, NULL);
  }
}

void SNetThreadingYield(void)
{
  LpelTaskYield();
}

snet_stream_t *SNetStreamCreate(int cap)
{
  return (snet_stream_t *) LpelStreamCreate(cap);
}

snet_stream_desc_t *SNetStreamOpen(snet_stream_t * stream, char mode)
{
  return (snet_stream_desc_t *) LpelStreamOpen((lpel_stream_t *) stream,
      mode);
}

snet_locvec_t *SNetStreamGetSource(snet_stream_t * s)
{
  usrdata_t *dat = LpelStreamGetUsrData((lpel_stream_t *) s);
  if (dat != NULL) {
    return dat->source;
  }
  return NULL;
}

void SNetStreamSetSource(snet_stream_t * s, snet_locvec_t * lv)
{
  usrdata_t *dat = CheckCreateUsrdata((lpel_stream_t *) s);
  dat->source = SNetLocvecCopy(lv);
}

void SNetStreamRegisterReadCallback(snet_stream_t * s,
    void (*callback) (void *), void *cbarg)
{
  usrdata_t *dat = CheckCreateUsrdata((lpel_stream_t *) s);
  dat->callback_read.func = callback;
  dat->callback_read.arg = cbarg;
}


void *SNetStreamGetCallbackArg(snet_stream_desc_t *sd)
{
  lpel_stream_t *ls = LpelStreamGet((lpel_stream_desc_t *) sd);
  usrdata_t *dat;

  assert(ls != NULL);

  dat = LpelStreamGetUsrData(ls);
  return dat->callback_read.arg;
}


void SNetStreamClose(snet_stream_desc_t * sd, int destroy_stream)
{
  if (destroy_stream) {
    /* also need to destroy the usrdata */
    DestroyUsrdata(LpelStreamGet((lpel_stream_desc_t *) sd));
  }
  LpelStreamClose((lpel_stream_desc_t *) sd, destroy_stream);
}

void SNetStreamReplace(snet_stream_desc_t * sd, snet_stream_t * new_stream)
{
  LpelStreamReplace((lpel_stream_desc_t *) sd,
      (lpel_stream_t *) new_stream);
}

snet_stream_t *SNetStreamGet(snet_stream_desc_t * sd)
{
  return (snet_stream_t *) LpelStreamGet((lpel_stream_desc_t *) sd);
}

void *SNetStreamRead(snet_stream_desc_t * sd)
{
  void *item = LpelStreamRead((lpel_stream_desc_t *) sd);
  usrdata_t *dat;

  /* call the read callback function */
  dat = LpelStreamGetUsrData(LpelStreamGet((lpel_stream_desc_t *) sd));
  if (dat != NULL) {
    if (dat->callback_read.func) {
      dat->callback_read.func(dat->callback_read.arg);
    }
  }

  return item;
}

void *SNetStreamPeek(snet_stream_desc_t * sd)
{
  return LpelStreamPeek((lpel_stream_desc_t *) sd);
}

void SNetStreamWrite(snet_stream_desc_t * sd, void *item)
{
  LpelStreamWrite((lpel_stream_desc_t *) sd, item);
}

int SNetStreamTryWrite(snet_stream_desc_t * sd, void *item)
{
  return LpelStreamTryWrite((lpel_stream_desc_t *) sd, item);
}

snet_stream_desc_t *SNetStreamPoll(snet_streamset_t * set)
{
  return (snet_stream_desc_t *) LpelStreamPoll((lpel_streamset_t *) set);
}

void SNetStreamsetPut(snet_streamset_t * set, snet_stream_desc_t * sd)
{
  LpelStreamsetPut((lpel_streamset_t *) set, (lpel_stream_desc_t *) sd);
}

int SNetStreamsetRemove(snet_streamset_t * set, snet_stream_desc_t * sd)
{
  return LpelStreamsetRemove((lpel_streamset_t *) set,
      (lpel_stream_desc_t *) sd);
}

snet_stream_iter_t *SNetStreamIterCreate(snet_streamset_t * set)
{
  return (snet_stream_iter_t *) LpelStreamIterCreate((lpel_streamset_t *)
      set);
}

void SNetStreamIterDestroy(snet_stream_iter_t * iter)
{
  LpelStreamIterDestroy((lpel_stream_iter_t *) iter);
}

void SNetStreamIterReset(snet_stream_iter_t * iter, snet_streamset_t * set)
{
  LpelStreamIterReset((lpel_stream_iter_t *) iter,
      (lpel_streamset_t *) set);
}

int SNetStreamIterHasNext(snet_stream_iter_t * iter)
{
  return LpelStreamIterHasNext((lpel_stream_iter_t *) iter);
}

snet_stream_desc_t *SNetStreamIterNext(snet_stream_iter_t * iter)
{
  return (snet_stream_desc_t *) LpelStreamIterNext((lpel_stream_iter_t *)
      iter);
}

void SNetStreamIterAppend(snet_stream_iter_t * iter, snet_stream_desc_t * node)
{
  LpelStreamIterAppend((lpel_stream_iter_t *) iter,
      (lpel_stream_desc_t *) node);
}

void SNetStreamIterRemove(snet_stream_iter_t * iter)
{
  LpelStreamIterRemove((lpel_stream_iter_t *) iter);
}
