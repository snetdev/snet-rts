#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include "arch/atomic.h"
#include "info.h"
#include "bool.h"
#include "memfun.h"

static int new_tag = 0;

struct snet_info {
  snet_info_tag_t tag;
  uintptr_t data;
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
  new->next = NULL;
  return new;
}

void SNetInfoSetTag(snet_info_t *info, snet_info_tag_t tag, uintptr_t data)
{
  while (info->tag != tag && info->tag != -1) {
    info = info->next;
  }

  if (info->tag == tag) {
    info->data = data;
  } else if (info->tag == -1) {
    info->tag = tag;
    info->data = data;
    info->next = SNetInfoInit();
  }
}

uintptr_t SNetInfoDelTag(snet_info_t *info, snet_info_tag_t tag)
{
  uintptr_t result;
  snet_info_t *tmp;

  while (info->next != NULL && info->next->tag != tag) {
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
  snet_info_t *tmp, *tmp2, *result;
  tmp = result = SNetInfoInit();
  tmp2 = info;
  do {
    tmp->tag = tmp2->tag;
    tmp->data = tmp2->data;
    tmp->next = tmp2->next ? SNetInfoInit() : NULL;
    tmp = tmp->next;
    tmp2 = tmp2->next;
  } while (tmp2 != NULL);
  return result;
}

void SNetInfoDestroy(snet_info_t *info)
{
  snet_info_t *tmp;
  while (info != NULL) {
    tmp = info->next;
    SNetMemFree(info);
    info = tmp;
  }
}
