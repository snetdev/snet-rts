#include <pthread.h>
#include "arch/atomic.h"
#include "info.h"
#include "bool.h"
#include "memfun.h"

static atomic_t new_tag = ATOMIC_INIT(0);

struct snet_info {
  pthread_mutex_t mutex;
  snet_info_tag_t tag;
  void *data;
  struct snet_info *next;
};

snet_info_tag_t SNetInfoCreateTag()
{
    return fetch_and_inc(&new_tag);
}

static snet_info_t *NewInfo()
{
  snet_info_t *new = SNetMemAlloc(sizeof(*new));
  new->tag = -1;
  new->data = NULL;
  new->next = NULL;
  return new;
}

snet_info_t *SNetInfoInit()
{
  snet_info_t *new = NewInfo();
  pthread_mutex_init(&new->mutex, NULL);
  return new;
}

void SNetInfoSetTag(snet_info_t *info, snet_info_tag_t tag, void *data)
{
  pthread_mutex_lock(&info->mutex);
  while (info->tag != tag && info->tag != -1) {
      info = info->next;
  }

  if (info->tag == tag) {
      info->data = data;
  } else if (info->tag == -1) {
      info->tag = tag;
      info->data = data;
      info->next = NewInfo();
  }
  pthread_mutex_unlock(&info->mutex);
}

void *SNetInfoDelTag(snet_info_t *info, snet_info_tag_t tag)
{
  void *result;
  snet_info_t *tmp;

  pthread_mutex_lock(&info->mutex);

  while (info->next != NULL && info->next->tag != tag) {
      info = info->next;
  }

  if (info->next == NULL) return NULL;

  tmp = info->next;
  info->next = tmp->next;
  result = tmp->data;
  SNetMemFree(tmp);
  pthread_mutex_unlock(&info->mutex);
  return result;
}

void *SNetInfoGetTag(snet_info_t *info, snet_info_tag_t tag)
{
  void *result = NULL;
  pthread_mutex_lock(&info->mutex);
  while (info->tag != tag && info->tag != -1) {
      info = info->next;
  }

  if (info->tag == tag) result = info->data;
  pthread_mutex_unlock(&info->mutex);
  return result;
}

snet_info_t *SNetInfoCopy(snet_info_t *info) {
    snet_info_t *tmp, *tmp2, *result;
    pthread_mutex_lock(&info->mutex);
    tmp = result = SNetInfoInit();
    tmp2 = info;
    do {
        tmp->tag = tmp2->tag;
        tmp->data = tmp2->data;
        tmp->next = tmp2->next ? NewInfo() : NULL;
        tmp = tmp->next;
        tmp2 = tmp2->next;
    } while (tmp2 != NULL);
    pthread_mutex_unlock(&info->mutex);
    return result;
}

void SNetInfoDestroy(snet_info_t *info)
{
  pthread_mutex_destroy(&info->mutex);
  snet_info_t *tmp;
  while (info != NULL) {
    tmp = info->next;
    SNetMemFree(info);
    info = tmp;
  }
}
