#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include "info.h"
#include "bool.h"
#include "memfun.h"

static int new_tag = 0;

struct snet_info {
  snet_info_tag_t tag;
  uintptr_t data;
  void *(*copyFun)(void *);
  struct snet_info *next;
};

snet_info_tag_t SNetInfoCreateTag()
{
  return new_tag++;
}

snet_info_t *SNetInfoInit()
{
  snet_info_t *new = SNetMemAlloc(sizeof(*new));
  new->tag = -1;
  new->data = 0;
  new->copyFun = NULL;
  new->next = NULL;
  return new;
}

void SNetInfoSetTag(snet_info_t *info, snet_info_tag_t tag, uintptr_t data,
                    void *(*copyFun)(void *))
{
  while (info->tag != tag && info->tag != -1) {
    info = info->next;
  }

  if (info->tag == tag) {
    if (copyFun) SNetMemFree((void*) info->data);
    info->data = data;
    info->copyFun = copyFun;
  } else if (info->tag == -1) {
    info->tag = tag;
    info->data = data;
    info->copyFun = copyFun;
    info->next = SNetInfoInit();
  }
}

uintptr_t SNetInfoDelTag(snet_info_t *info, snet_info_tag_t tag)
{
  uintptr_t result;
  snet_info_t *tmp;

  while (info->next && info->next->tag != tag) {
    info = info->next;
  }

  assert(info->next != NULL);

  tmp = info->next;
  info->next = tmp->next;
  result = tmp->data;
  SNetMemFree(tmp);
  return result;
}

uintptr_t SNetInfoGetTag(snet_info_t *info, snet_info_tag_t tag)
{
  while (info->tag != tag && info->tag != -1) {
    info = info->next;
  }

  assert(info->tag == tag);

  return info->data;
}

snet_info_t *SNetInfoCopy(snet_info_t *info)
{
  snet_info_t *from, *to;
  snet_info_t *result = SNetInfoInit();

  from = info;
  to = result;

  do {
    to->tag = from->tag;
    to->copyFun = from->copyFun;

    if (to->copyFun) to->data = (uintptr_t) from->copyFun((void*) from->data);
    else to->data = from->data;

    to->next = from->next ? SNetInfoInit() : NULL;

    to = to->next;
    from = from->next;
  } while (from);

  return result;
}

void SNetInfoDestroy(snet_info_t *info)
{
  snet_info_t *tmp;
  while (info) {
    tmp = info->next;
    if (info->copyFun) SNetMemFree((void*) info->data);
    SNetMemFree(info);
    info = tmp;
  }
}
